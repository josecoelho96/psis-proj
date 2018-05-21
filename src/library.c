#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "clipboard.h"
#include "communications.h"

int clipboard_connect(char *clipboard_dir) {

	int fd;
    char clipboard_path[256];
    struct sockaddr_un addr;

    snprintf(clipboard_path, 256, "%s%s", clipboard_dir, "CLIPBOARD_SOCKET");


	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("Error [socket]");
        return -1;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, clipboard_path);

	if (connect(fd, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("Error [connect]");
        return -1;
	}
    return fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {


    return unix_stream_write(clipboard_id, buf, count);
}