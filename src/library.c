#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
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

    char header_msg[sizeof(header_t)];
    size_t bytes_sent;
    header_t header;

    if (region < 0 || region > 9) {
        printf("Error: Invalid region selected.\n");
        return 0;
    }

    if (count <= 0) {
        printf("You must copy a positive number of bytes.\n");
        return 0;
    }
    // send message header
    header.operation = OPERATION_COPY;
    header.region = region;
    header.count = count;
    memcpy(header_msg, &header, sizeof(header_t));
    bytes_sent = send(clipboard_id, header_msg, sizeof(header_t), 0);
    if (bytes_sent <= 0) {
        printf("[DEBUG] Error sending header\n");
        return 0;
    }
    bytes_sent = send(clipboard_id, buf, count, 0);
    return bytes_sent > 0 ? bytes_sent : 0;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {

    char header[10];
    char header_recv[9];
    char dest[8];
    int content_size;
    int bytes_recv;

    if (region < 0 || region > 9) {
        printf("Error: Invalid region selected.\n");
        return 0;
    }

    if (count <= 0) {
        printf("You must paste a positive number of bytes.\n");
        return 0;
    }

    sprintf(header, "p%c%ld", region+'0', count);
    send(clipboard_id, header, 10, 0);
    recv(clipboard_id, header_recv, 9, 0);

    // TODO: change from int to size_t
    strncpy(dest, &header_recv[1], 8);
    content_size = atoi(dest);

    printf("Received header: %.*s. Reading %d bytes.\n", 9, header_recv, content_size);

    if (content_size == 0) {
        printf("No data is present on the selected region.\n");
        return 0;
    }
    bytes_recv = recv(clipboard_id, buf, content_size, 0);
    return bytes_recv > 0 ? bytes_recv : 0;
}