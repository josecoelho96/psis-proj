#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "clipboard.h"
#include "communication.h"
#include "protocol.h"

int clipboard_connect(char *clipboard_dir) {

	int fd;
    char clipboard_path[256];
    struct sockaddr_un addr;

    snprintf(clipboard_path, 256, "%s%s", clipboard_dir, "CLIPBOARD_SOCKET");
    if (init_stream_unix(&fd, &addr, clipboard_path) == -1) {
        printf("Error connecting to clipboard\n");
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

    char header_msg[sizeof(header_t)];
    header_t header;


    int bytes_recv;

    if (region < 0 || region > 9) {
        printf("Error: Invalid region selected.\n");
        return 0;
    }

    if (count <= 0) {
        printf("You must paste a positive number of bytes.\n");
        return 0;
    }


    // send message header
    header.operation = OPERATION_PASTE;
    header.region = region;
    header.count = count;

    memcpy(header_msg, &header, sizeof(header_t));

    send(clipboard_id, header_msg, sizeof(header_t), 0);
    recv(clipboard_id, header_msg, sizeof(header_t), 0);

    memcpy(&header, header_msg, sizeof(header_t));

    printf("[DEBUG] Operation: %c\n", header.operation);
    printf("[DEBUG] Region: %d\n", header.region);
    printf("[DEBUG] Length: %ld\n", header.count);

    if (header.count == 0) {
        printf("No data is present on the selected region.\n");
        return 0;
    }
    bytes_recv = recv(clipboard_id, buf, header.count, 0);
    return bytes_recv > 0 ? bytes_recv : 0;
}