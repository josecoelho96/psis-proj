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
    int ls_fd = args->fd;
    int app_fd;
    struct sockaddr_un app_addr;
    pthread_t recv_data_app_id;
    recv_data_app_args_t new_args;

    // printf("[DEBUG] New thread: recv_conn_app. fd=%d\n", ls_fd);

    while(1) {
        if ((app_fd = stream_unix_accept(ls_fd, &app_addr)) == -1) {
            printf("Error accepting new connection from app.\n");
            continue; // next cycle iteration
        }
        // printf("[DEBUG][recv_conn_app] New app connected (fd: %d)\n", app_fd);

        // Create new thread for the newly connected app
        new_args.fd = app_fd;
        new_args.regions = args->regions;
        new_args.parent_clip_fd = args->parent_clip_fd;
        new_args.children_clip_connections = args->children_clip_connections;
        new_args.region_locks = args->region_locks;
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
    connection_t **clip_children = (connection_t **)args->children_clip_connections;
    data_region_t *regions = (data_region_t *)args->regions;
    int fd = args->fd;
    char *content;
    pthread_rwlock_t **locks = (pthread_rwlock_t **)args->region_locks;

    // printf("[DEBUG] New thread: recv_data_app. fd=%d\n", fd);

    while(1) {

        // receive header
        if (recv_header(fd, &header) <= 0) {
            // TODO: Handle return
            printf("Error when reading header from app.\n");
            pthread_exit(NULL);
        }

        if (header.operation == OPERATION_COPY ) {
            // printf("[DEBUG][recv_data_app] Operation: Copy\n");

            if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
                perror("Error [malloc]");
                pthread_exit(NULL);
            }
            if (recv_content(fd, content, header.count) == -1) {
                // TODO: Handle return
                printf("Error when reading content from app.\n");
                pthread_exit(NULL);
            }

            // Update region
            // lock for writing
            pthread_rwlock_wrlock(locks[header.region]);
            update_region(content, header.count, &(regions[header.region]));
            // unlock
            pthread_rwlock_unlock(locks[header.region]);
            // printf("[DEBUG][recv_data_app] Operation: %c.\n", header.operation);
            // printf("[DEBUG][recv_data_app] Region: %d.\n", header.region);
            // printf("[DEBUG][recv_data_app] Count: %ld.\n", header.count);

            // Update all other clipboards connected
            header.operation = OPERATION_UPDATE;

            if (*(args->parent_clip_fd) != -1) {
                // Not the root, send to parent
                // printf("[DEBUG][recv_data_app] (%d) Not root. Send new content to parent\n", *(args->parent_clip_fd));
                // lock for reading
                pthread_rwlock_rdlock(locks[header.region]);
                if (send_message(*(args->parent_clip_fd), header, regions[header.region]) == -1 ) {
                    // TODO: Handle return
                    printf("Error when sending message to parent clipboard.\n");
                    // unlock for reading
                    pthread_rwlock_unlock(locks[header.region]);
                    pthread_exit(NULL);
                }
                // unlock for reading
                pthread_rwlock_unlock(locks[header.region]);
            } else {
                // Root, send to children, if any
                // printf("[DEBUG][recv_data_app] (%d) Root. Send new content to children\n", *(args->parent_clip_fd));
                // lock for reading
                pthread_rwlock_rdlock(locks[header.region]);
                if (update_children(*clip_children, header, regions[header.region]) == -1 ) {
                    // TODO: Handle return
                    printf("Error when sending updates to children.\n");
                    // unlock for reading
                    pthread_rwlock_unlock(locks[header.region]);
                    pthread_exit(NULL);
                }
                // unlock for reading
                pthread_rwlock_unlock(locks[header.region]);
            }
        } else if (header.operation == OPERATION_PASTE ) {
            // printf("[DEBUG][recv_data_app] Operation: Paste\n");
            // printf("[DEBUG] Paste %ld bytes (max) from region %d\n", header.count, header.region);
            // printf("[DEBUG] Actual region data size: %ld\n", regions[header.region].size);

            header.count = regions[header.region].size > header.count ? header.count : regions[header.region].size;
            // printf("[DEBUG] Actual paste: %ld\n", header.count);

            // lock for reading
            pthread_rwlock_rdlock(locks[header.region]);
            if (send_message(fd, header, regions[header.region]) == -1 ) {
            // TODO: Handle return
                printf("Error when sending message to parent clipboard.\n");
                // unlock for reading
                pthread_rwlock_unlock(locks[header.region]);
                pthread_exit(NULL);
            }
            // unlock for reading
            pthread_rwlock_unlock(locks[header.region]);
        } else {
            printf("[recv_data_app] Operation: Invalid\n");
            // printf("[DEBUG][recv_data_app] Operation: Invalid\n");
        }
    }
    return NULL;
}

void *recv_conn_clip(void *arg) {

    recv_conn_clip_args_t *args = arg;
    struct sockaddr_in clip_addr;
    int clip_fd;
    pthread_t recv_data_clip_id;
    recv_data_clip_args_t new_args;
    connection_t *new_connection;
    connection_t **children = (connection_t **)args->children_connections;
    int ls_fd = args->fd;
    pthread_rwlock_t **locks = (pthread_rwlock_t **)args->region_locks;
    data_region_t *regions = (data_region_t *)args->regions;
    header_t header;

    // printf("[DEBUG] New thread: recv_conn_clip. fd=%d\n", ls_fd);

    while(1) {

        if ((clip_fd = tcp_accept(ls_fd, &clip_addr)) == -1) {
            printf("Error accepting new connection from clipboard.\n");
            continue; // next cycle iteration
        }
        // printf("[DEBUG][recv_conn_clip] New clipboard connected (fd: %d)\n", clip_fd);

        // Create new thread for the newly connected clip
        new_args.fd = clip_fd;
        new_args.regions = args->regions;
        new_args.children_connections = args->children_connections;
        new_args.parent_clip_fd = args->parent_clip_fd;
        new_args.region_locks = args->region_locks;
        if (pthread_create(&recv_data_clip_id, NULL, recv_data_clip, &new_args) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }

        // Add to the children list of current clipboard
        if ((new_connection = (connection_t *)malloc(sizeof(connection_t))) == NULL) {
            perror("Error [malloc]");
            pthread_exit(NULL);
        }

        // TODO: Add rwlock to children list
        new_connection->fd = clip_fd;
        new_connection->next = *children;
        *children = new_connection;

        // Send updated content to new clipboard
        // printf("[DEBUG][recv_conn_clip] New connection fd: %d\n", clip_fd);

        header.operation = OPERATION_UPDATE;
        for (int i = 0; i < REGIONS_QUANTITY; i++) {
            // lock for reading
            pthread_rwlock_rdlock(locks[i]);
            if (regions[i].size == 0) {
                pthread_rwlock_unlock(locks[i]);
                continue;
            }
            header.region = i;
            header.count = regions[i].size;
            if (send_message(clip_fd, header, regions[i]) == -1) {
                printf("Error sending message.\n");
                // unlock for reading
                pthread_rwlock_unlock(locks[i]);
                pthread_exit(NULL);
            }
            // unlock for reading
            pthread_rwlock_unlock(locks[i]);
        }
    }
    return NULL;
}

void *recv_data_clip(void *arg) {

    recv_data_clip_args_t *args = arg;
    header_t header;
    connection_t **children = (connection_t **)args->children_connections;
    data_region_t *regions = (data_region_t *)args->regions;
    char *content;
    int fd = args->fd;
    pthread_rwlock_t **locks = (pthread_rwlock_t **)args->region_locks;

    // printf("[DEBUG] New thread: recv_data_clip. fd=%d\n", fd);

    while(1) {

        // receive header
        if (recv_header(fd, &header) <= 0) {
            // TODO: Handle return
            printf("Error when reading header from clipboard.\n");
            pthread_exit(NULL);
        }
        // printf("[DEBUG][recv_data_clip] Received header from child.\n");
        // printf("[DEBUG][recv_data_clip] Operation: %c.\n", header.operation);
        // printf("[DEBUG][recv_data_clip] Region: %d.\n", header.region);
        // printf("[DEBUG][recv_data_clip] Count: %ld.\n", header.count);

        if (header.operation == OPERATION_UPDATE) {
            // printf("[DEBUG][recv_data_clip] Operation: UPDATE.\n");
            // printf("[DEBUG][recv_data_clip] Region: %d.\n", header.region);
            // printf("[DEBUG][recv_data_clip] Count: %ld.\n", header.count);

            // receive content from parent and update regions
            if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
                perror("Error [malloc]");
                pthread_exit(NULL);
            }
            if (recv_content(fd, content, header.count) == -1) {
                // TODO: Handle return
                printf("Error when reading content from clipboard.\n");
                pthread_exit(NULL);
            }
            // printf("[DEBUG][recv_data_clip] Content received.\n");

            // Update region
            // lock for reading
            pthread_rwlock_rdlock(locks[header.region]);
            update_region(content, header.count, &(regions[header.region]));
            // unlock for reading
            pthread_rwlock_unlock(locks[header.region]);
            // Not root, send content to parent
            if (*(args->parent_clip_fd) != -1) {
                // Not the root, send to parent
                // printf("[DEBUG][recv_data_clip] Not root. Send new content to parent\n");
                // lock for reading
                pthread_rwlock_rdlock(locks[header.region]);
                if (send_message(*(args->parent_clip_fd), header, regions[header.region]) == -1 ) {
                    // TODO: Handle return
                    printf("Error when sending message to parent clipboard.\n");
                    // unlock for reading
                    pthread_rwlock_unlock(locks[header.region]);
                    pthread_exit(NULL);
                }
                // unlock for reading
                pthread_rwlock_unlock(locks[header.region]);
            } else {
                // Root, send to children, if any
                // printf("[DEBUG][recv_data_clip] Root. Send new content to children\n");
                // lock for reading
                pthread_rwlock_rdlock(locks[header.region]);
                if (update_children(*children, header, regions[header.region]) == -1 ) {
                    // TODO: Handle return
                    printf("Error when sending updates to children.\n");
                    // unlock for reading
                    pthread_rwlock_unlock(locks[header.region]);
                    pthread_exit(NULL);
                }
                // unlock for reading
                pthread_rwlock_unlock(locks[header.region]);
            }
        } else {
            printf("[recv_data_clip] Operation: Invalid\n");
        }
    }
    return NULL;
}

void *recv_data_parent_clip(void *arg) {

    recv_data_parent_clip_args_t *args = arg;
    connection_t **clip_children = (connection_t **)(args->children_clip_connections);
    header_t header;
    data_region_t *regions = (data_region_t *)args->regions;
    char *content;
    int fd = args->fd;
    pthread_rwlock_t **locks = (pthread_rwlock_t **)args->region_locks;

    while(1) {

        // receive header
        if (recv_header(fd, &header) <= 0) {
            // TODO: Handle return
            printf("Error when reading header parent clipboard.\n");
            pthread_exit(NULL);
        }
        // printf("[DEBUG][recv_data_parent_clip] Received header from parent.\n");
        // printf("[DEBUG][recv_data_parent_clip] Operation: %c.\n", header.operation);
        // printf("[DEBUG][recv_data_parent_clip] Region: %d.\n", header.region);
        // printf("[DEBUG][recv_data_parent_clip] Count: %ld.\n", header.count);

        if (header.operation == OPERATION_UPDATE) {
            // receive content from parent and update regions
            if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
                perror("Error [malloc]");
                pthread_exit(NULL);
            }
            if (recv_content(fd, content, header.count) == -1) {
                // TODO: Handle return
                printf("Error when reading content from parent clipboard.\n");
                pthread_exit(NULL);
            }

            // lock for writing
            pthread_rwlock_wrlock(locks[header.region]);
            update_region(content, header.count, &(regions[header.region]));
            // unlock
            pthread_rwlock_unlock(locks[header.region]);

            // Send content to children, if any
            // lock for reading
            pthread_rwlock_rdlock(locks[header.region]);
            if (update_children(*clip_children, header, regions[header.region]) == -1 ) {
                // TODO: Handle return
                printf("Error when sending updates to children.\n");
                // unlock
                pthread_rwlock_unlock(locks[header.region]);
                pthread_exit(NULL);
            }
            // unlock
            pthread_rwlock_unlock(locks[header.region]);
        } else {
            printf("[recv_data_parent_clip] Operation: Invalid\n");
        }
    }
    return NULL;
}