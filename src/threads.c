#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>

#include "communication.h"
#include "protocol.h"
#include "threads.h"


void *recv_conn_app(void *arg) {

    recv_conn_app_args_t *args = arg;
    int app_fd;
    struct sockaddr_un app_addr;
    pthread_t recv_data_app_id;
    recv_data_app_args_t new_args;

    while(1) {
        if ((app_fd = stream_unix_accept(args->fd, &app_addr)) == -1) {
            printf("Error accepting new connection from app.\n");
            continue; // next cycle iteration
        }
        printf("[DEBUG][recv_conn_app] New app connected (fd: %d)\n", app_fd);

        // Create new thread for the newly connected app
        new_args.fd = app_fd;
        new_args.regions = args->regions;
        new_args.parent_clip_fd = args->parent_clip_fd;
        new_args.children_clip_connections = args->children_clip_connections;
        if (pthread_create(&recv_data_app_id, NULL, recv_data_app, &new_args) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void *recv_data_app(void *arg) {

    recv_data_app_args_t *args = arg;
    header_t header;
    connection_t **clip_children = (connection_t **)(args->children_clip_connections);

    printf("[DEBUG][recv_data_app] New app thread (fd: %d)\n", args->fd);
    while(1) {

        // receive header
        if (recv_header(args->fd, &header) != 0) {
            // TODO: Handle return
            printf("Error when reading header from app.\n");
            pthread_exit(NULL);
        }

        if (header.operation == OPERATION_COPY ) {
            printf("[DEBUG][recv_data_app] Operation: Copy\n");

            if (recv_content(args->fd, header, args->regions) != 0) {
                // TODO: Handle return
                printf("Error when reading content from app.\n");
                pthread_exit(NULL);
            }
            // Update all other clipboards connected
            header.operation = OPERATION_UPDATE;

            if (*(args->parent_clip_fd) != -1) {
                // Not the root, send to parent
                printf("[DEBUG][recv_data_app] (%d) Not root. Send new content to parent\n", *(args->parent_clip_fd));

                if (send_message(*(args->parent_clip_fd), header, args->regions) != 0 ) {
                    // TODO: Handle return
                    printf("Error when sending message to parent clipboard.\n");
                    pthread_exit(NULL);
                }

            } else {
                // Root, send to children, if any
                printf("[DEBUG][recv_data_app] (%d) Root. Send new content to children\n", *(args->parent_clip_fd));

                if (update_children(*clip_children, header, args->regions) != 0 ) {
                    // TODO: Handle return
                    printf("Error when sending updates to children.\n");
                    pthread_exit(NULL);
                }
            }
        } else if (header.operation == OPERATION_PASTE ) {
            printf("[DEBUG][recv_data_app] Operation: Paste\n");

            // // printf("[DEBUG] Paste %d bytes (max) from region %d\n", content_size, region);
            // // printf("[DEBUG] Actual region data size: %ld\n", regions[region].size);

            // bytes_paste = regions[header.region].size > header.count ? header.count : regions[header.region].size;
            // printf("[DEBUG] Actual paste: %ld\n", bytes_paste);

            // header.count = bytes_paste;

            // memcpy(header_msg, &header, sizeof(header_t));

            // send(*(int *)fd, header_msg, sizeof(header_t), 0);
            // send(*(int *)fd, regions[header.region].content, bytes_paste, 0);
        } else {
            printf("[DEBUG][recv_data_app] Operation: Invalid\n");
        }
    }
    return NULL;
}

void *recv_conn_clip(void *arg) {

    recv_conn_clip_args_t *args = arg;
    struct sockaddr_in clip_addr;
    int clip_fd = 0;
    pthread_t recv_data_clip_id;
    recv_data_clip_args_t new_args;
    connection_t *new_connection;
    connection_t **children = (connection_t **)args->children_connections;

    while(1) {

        if ((clip_fd = tcp_accept(args->fd, &clip_addr)) == -1) {
            printf("Error accepting new connection from clipboard.\n");
            continue; // next cycle iteration
        }
        printf("[DEBUG][recv_conn_clip] New clipboard connected (fd: %d)\n", clip_fd);

        // Create new thread for the newly connected clip
        new_args.fd = clip_fd;
        new_args.regions = args->regions;
        new_args.children_connections = args->children_connections;
        new_args.parent_clip_fd = args->parent_clip_fd;
        if (pthread_create(&recv_data_clip_id, NULL, recv_data_clip, &new_args) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }

        // Add to the children list of current clipboard
        // TODO: Improve: Check if some node is invalid and use it instead.
        if ((new_connection = (connection_t *)malloc(sizeof(connection_t))) == NULL) {
            perror("Error [malloc]");
            pthread_exit(NULL);
        }

        // TODO: Add rwlock or mutex (sync stuff)
        new_connection->fd = clip_fd;
        new_connection->next = *children;
        *children = new_connection;

        // Send updated content to new clipboard
        printf("[DEBUG][recv_conn_clip] New connection fd: %d\n", clip_fd);
        if (send_regions(clip_fd, (data_region_t *)args->regions) != 0) {
            printf("Error sending updated regions to clipboard.\n");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void *recv_data_clip(void *arg) {

    recv_data_clip_args_t *args = arg;
    header_t header;
    connection_t **children = (connection_t **)args->children_connections;

    while(1) {

        // receive header
        if (recv_header(args->fd, &header) != 0) {
            // TODO: Handle return
            printf("Error when reading header from clipboard.\n");
            pthread_exit(NULL);
        }
        printf("[DEBUG][recv_data_clip] Received header from child.\n");
        printf("[DEBUG][recv_data_clip] Operation: %c.\n", header.operation);
        printf("[DEBUG][recv_data_clip] Region: %d.\n", header.region);
        printf("[DEBUG][recv_data_clip] Count: %ld.\n", header.count);

        if (header.operation == OPERATION_UPDATE) {
            printf("[DEBUG][recv_data_clip] Operation: UPDATE.\n");
            // receive content from parent and update regions
            if (recv_content(args->fd, header, args->regions) != 0) {
                // TODO: Handle return
                printf("Error when reading content from clipboard.\n");
                pthread_exit(NULL);
            }
            // printf("[DEBUG][recv_data_clip]Content received.\n");

            // Not root, send content to parent
            if (*(args->parent_clip_fd) != -1) {
                // Not the root, send to parent
                printf("[DEBUG][recv_data_clip] Not root. Send new content to parent\n");

                if (send_message(*(args->parent_clip_fd), header, args->regions) != 0 ) {
                    // TODO: Handle return
                    printf("Error when sending message to parent clipboard.\n");
                    pthread_exit(NULL);
                }

            } else {
                // Root, send to children, if any
                printf("[DEBUG][recv_data_clip] Root. Send new content to children\n");

                if (update_children(*children, header, args->regions) != 0 ) {
                    // TODO: Handle return
                    printf("Error when sending updates to children.\n");
                    pthread_exit(NULL);
                }
            }
        } else {
            printf("[DEBUG][recv_data_clip] Operation: Invalid\n");
        }
    }
    return NULL;
}

void *recv_data_parent_clip(void *arg) {

    recv_data_parent_clip_args_t *args = arg;
    connection_t **clip_children = (connection_t **)(args->children_clip_connections);
    header_t header;

    while(1) {

        // receive header
        if (recv_header(args->fd, &header) != 0) {
            // TODO: Handle return
            printf("Error when reading header from app.\n");
            pthread_exit(NULL);
        }
        printf("[DEBUG][recv_data_parent_clip] Received header from parent.\n");

        if (header.operation == OPERATION_UPDATE) {
            // receive content from parent and update regions
            if (recv_content(args->fd, header, args->regions) != 0) {
                // TODO: Handle return
                printf("Error when reading content from app.\n");
                pthread_exit(NULL);
            }
            // Send content to children, if any
            if (update_children(*clip_children, header, args->regions) != 0 ) {
                // TODO: Handle return
                printf("Error when sending updates to children.\n");
                pthread_exit(NULL);
            }
        } else {
            printf("[DEBUG][recv_data_parent_clip] Operation: Invalid\n");
        }
    }
    return NULL;
}