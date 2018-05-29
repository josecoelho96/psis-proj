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
    ret_val = recv_data(fd, header_msg, sizeof(header_t));
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
    ret_val = recv_data(fd, content, header.count);
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }

    printf("[DEBUG][recv_content] Received: %.*s\n", (int)header.count, content);
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
    ret_val = send_data(fd, header_msg, sizeof(header_t));
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }
    return 0;
}

int send_content(int fd, header_t header, data_region_t *regions) {

    int ret_val;

    ret_val = send_data(fd, regions[header.region].content, header.count);
    if (ret_val != 1) {
        // error. return immediately
        return -1;
    }

    return 0;
}

int send_message(int fd, header_t header, data_region_t *regions) {

    if (send_header(fd, header) != 0) {
        printf("Error sending header.\n");
        return -1;
    }

    if (send_content(fd, header, regions) != 0) {
        printf("Error sending content.\n");
        return -1;
    }
    return 0;
}

int send_regions(int fd, data_region_t *regions) {

    header_t header;
    header.operation = OPERATION_UPDATE;
    printf("[DEBUG][send_regions] Send regions informations.\n");

    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        printf("[DEBUG] Sending region %d to fd %d\n", i, fd);
        header.region = i;
        header.count = regions[i].size;
        if (send_message(fd, header, regions) != 0) {
            printf("Error sending message.\n");
            return -1;
        }
    }
    return 0;
}

int update_children(connection_t *head, header_t header, data_region_t *regions) {

    connection_t *aux = head;

    while (aux!= NULL) {
        if (aux->fd > 0) {
            if (send_message(aux->fd, header, regions) != 0 ) {
                // TODO: Handle return
                printf("Error when sending message to children.\n");
                return -1;
            }
        }
        aux = aux->next;
    }
    return 0;
}