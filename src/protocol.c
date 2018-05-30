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
    if (ret_val <= 0) {
        // TODO: error. return immediately
        return -1;
    }
    memcpy(header, header_msg, sizeof(header_t));
    return ret_val;
}

int recv_content(int fd, char **content, size_t count) {

    char *cont;
    int ret_val;
    if ((cont = (char *)malloc(count * sizeof(char))) == NULL) {
        perror("Error [malloc]");
        return -1;
    }
    ret_val = recv_data(fd, cont, count);
    if (ret_val <= 0) {
        // error. return immediately
        return -1;
    }

    *content = cont;
    return ret_val;
}

int send_header(int fd, header_t header) {

    int ret_val;
    char header_msg[sizeof(header_t)];
    memcpy(header_msg, &header, sizeof(header_t));
    ret_val = send_data(fd, header_msg, sizeof(header_t));
    if (ret_val <= 0) {
        // error. return immediately
        return -1;
    }
    return ret_val;
}

int send_content(int fd, char *content, size_t count) {

    int ret_val;
    ret_val = send_data(fd, content, count);
    if (ret_val <= 0) {
        // error. return immediately
        return -1;
    }
    return ret_val;
}

int send_message(int fd, header_t header, data_region_t region) {

    int ret_val;
    if (send_header(fd, header) == -1) {
        printf("Error sending header.\n");
        return -1;
    }

    if ((ret_val = send_content(fd, region.content, header.count)) == -1) {
        printf("Error sending content.\n");
        return -1;
    }
    return ret_val;
}

int send_regions(int fd, data_region_t *regions) {

    header_t header;
    header.operation = OPERATION_UPDATE;

    for (int i = 0; i < REGIONS_QUANTITY; i++) {

        if (regions[i].size == 0) {
            continue;
        }
        header.region = i;
        header.count = regions[i].size;
        if (send_message(fd, header, regions[i]) == -1) {
            printf("Error sending message.\n");
            return -1;
        }
    }
    return 0;
}

int update_region(char *content, size_t count, data_region_t *region) {

    // update regions
    if (region->size > count) {
        // space previously allocated is bigger than needed
        memcpy(region->content, content, count);
    } else {
        // more space is needed
        free(region->content);
        region->content = content;
    }
    region->size = count;
    return 0;
}

int update_children(connection_t *head, header_t header, data_region_t region) {

    connection_t *aux = head;
    while (aux!= NULL) {
        if (aux->fd > 0) {
            if (send_message(aux->fd, header, region) == -1 ) {
                // TODO: Handle return
                printf("Error when sending message to children.\n");
                return -1;
            }
        }
        aux = aux->next;
    }
    return 0;
}