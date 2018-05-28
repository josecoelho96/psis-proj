#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "communication.h"
#include "protocol.h"

int recv_header(int fd, header_t *header) {

    int ret_val;
    char header_msg[sizeof(header_t)];
    ret_val = stream_unix_recv(fd, header_msg, sizeof(header_t));
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }
    memcpy(header, header_msg, sizeof(header_t));
    return 0;
}

int recv_content(int fd, header_t header, data_region_t *regions) {

    char *content;
    int ret_val;
    if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
        perror("Error [malloc]");
        return -1;
    }
    ret_val = stream_unix_recv(fd, content, header.count);
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }
    // update regions
    if (regions[header.region].size > header.count) {
        // space previously allocated is bigger than needed
        memcpy(regions[header.region].content, content, header.count);
    } else {
        // more space is needed
        free(regions[header.region].content);
        regions[header.region].content = content;
    }
    regions[header.region].size = header.count;

    return 0;
}

int send_header(int fd, header_t header) {

    int ret_val;
    char header_msg[sizeof(header_t)];
    memcpy(header_msg, &header, sizeof(header_t));
    ret_val = stream_unix_send(fd, header_msg, sizeof(header_t));
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }
    return 0;
}

int send_content(int fd, header_t header, data_region_t *regions) {

    int ret_val;

    ret_val = stream_unix_send(fd, regions[header.region].content, header.count);
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }

    return 0;
}