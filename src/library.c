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

    header_t header;
    int ret_val;

    if (region < 0 || region > 9) {
        printf("Error: Invalid region selected.\n");
        return 0;
    }

    if (count <= 0) {
        printf("You must copy a positive number of bytes.\n");
        return 0;
    }

    header.operation = OPERATION_COPY;
    header.region = region;
    header.count = count;

    if (send_header(clipboard_id, header) == -1) {
        printf("Error sending header.\n");
        return 0;
    }

    if ((ret_val = send_content(clipboard_id, buf, count)) == -1) {
        printf("Error sending content.\n");
        return 0;
    }
    return ret_val;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {

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

    header.operation = OPERATION_PASTE;
    header.region = region;
    header.count = count;

    if (send_header(clipboard_id, header) == -1) {
        printf("Error sending header.\n");
        return 0;
    }

    // receive response header
    if (recv_header(clipboard_id, &header) <= 0) {
        printf("Error receiving header.\n");
        return 0;
    }

    if (header.count == 0) {
        printf("No data is present on the selected region.\n");
        return 0;
    }

    if ((bytes_recv = recv_content(clipboard_id, buf, header.count)) == -1) {
        printf("Error receiving content.\n");
        return 0;
    }
    return bytes_recv;
}

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count) {
    // TODO:
    return 0;
}

void clipboard_close(int clipboard_id) {
    close_stream_unix(clipboard_id);
}