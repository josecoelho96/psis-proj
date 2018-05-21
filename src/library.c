#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "clipboard.h"

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

    if (region < 0 || region > 9) {
        printf("Error: Invalid region selected.\n");
        return 0;
    }
    // send message header
    char header[11];
    sprintf(header, "c%c%ld", region+'0', count);
    send(clipboard_id, header, 10, 0);
    return send(clipboard_id, buf, count, 0);
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {

    char header[11];
    sprintf(header, "p%c%ld", region+'0', count);
    return send(clipboard_id, header, 10, 0);
}