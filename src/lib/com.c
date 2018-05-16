#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include "com.h"

int unix_stream_init_server(int *fd, struct sockaddr_un *addr, char *dir) {

    if ((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, dir);

    if (bind(*fd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
        perror("Error [bind]");
        return -1;
    }

    if(listen(*fd, 5) == -1) {
        perror("Error [listen]");
        return -1;
    }

    return 0;
}

int unix_stream_close(char *dir) {
    if (unlink(dir) == -1) {
        perror("Error [unlink]");
        return -1;
    }
    return 0;
}