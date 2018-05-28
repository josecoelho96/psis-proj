#include <pthread.h>
#include <stdio.h>
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

    printf("[DEBUG][recv_data_app] New app thread (fd: %d)\n", args->fd);
    while(1) {

        // receive header
        if (recv_header(args->fd, &header) != 0) {
            // TODO: Handle return
            printf("Error when reading header from app.\n");
            pthread_exit(NULL);
        }

        switch (header.operation) {
            case OPERATION_COPY:
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
                    printf("[DEBUG] (%d) Not root. Send new content to parent\n", *(args->parent_clip_fd));
                    if (send_header(*(args->parent_clip_fd), header) != 0) {
                        // TODO: Handle return
                        printf("Error when sending header to parent clipboard.\n");
                        pthread_exit(NULL);
                    }
                    // Send content
                    if (send_content(*(args->parent_clip_fd), header, args->regions) != 0) {
                        // TODO: Handle return
                        printf("Error when sending content to parent clipboard.\n");
                        pthread_exit(NULL);
                    }
                } else {
                    // Root, send to children, if any
                    printf("[DEBUG] (%d) Root. Send new content to children\n", *(args->parent_clip_fd));
                    // if (clip_connections != NULL) {
                    //     // send messages to other clipboards.
                    //     printf("Sending messages to leaves\n");
                    //     aux = clip_connections;
                    //     while(aux != NULL) {
                    //         //send to other clipboards
                    //         if (aux->fd > 0) {
                    //             // send header
                    //             nbytes = 0;
                    //             while (nbytes < sizeof(header_t)) {
                    //                 nbytes += send(aux->fd, header_msg, sizeof(header_t), 0);
                    //             }
                    //             //send content
                    //             nbytes = 0;
                    //             while (nbytes < header.count) {
                    //                 nbytes += send(aux->fd, content, header.count, 0);
                    //             }
                    //         }
                    //         aux = aux->next;
                    //     }
                    // }
                }

                break;
            case OPERATION_PASTE:
                // printf("[DEBUG] Operation: Paste\n");

                // // printf("[DEBUG] Paste %d bytes (max) from region %d\n", content_size, region);
                // // printf("[DEBUG] Actual region data size: %ld\n", regions[region].size);

                // bytes_paste = regions[header.region].size > header.count ? header.count : regions[header.region].size;
                // printf("[DEBUG] Actual paste: %ld\n", bytes_paste);

                // header.count = bytes_paste;

                // memcpy(header_msg, &header, sizeof(header_t));

                // send(*(int *)fd, header_msg, sizeof(header_t), 0);
                // send(*(int *)fd, regions[header.region].content, bytes_paste, 0);
                break;
            default:
                printf("[DEBUG][recv_data_app] Operation: Invalid\n");
                break;
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

    // pthread_t thr_id_conn_clip;
    // socklen_t addrlen = sizeof(clip_addr);
    // connection_t *new_connection = NULL;
    // size_t nbytes;
    // header_t header;
    // char header_msg[sizeof(header_t)];

    while(1) {

        if ((clip_fd = tcp_accept(args->fd, &clip_addr)) == -1) {
            printf("Error accepting new connection from clipboard.\n");
            continue; // next cycle iteration
        }
        printf("[DEBUG][recv_conn_app] New clipboard connected (fd: %d)\n", clip_fd);

        // Create new thread for the newly connected app
        new_args.fd = clip_fd;
        if (pthread_create(&recv_data_clip_id, NULL, recv_data_clip, &new_args) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }



        // TODO: ADD!
        // if ((new_connection = (connection_t*)malloc(sizeof(connection_t))) == NULL) {
        //     perror("Error [malloc]");
        //     pthread_exit(NULL);
        // };

        // // add new connection to list
        // // TODO: Improve: Check if some node is invalid.
        // new_connection->fd = clip_fd;
        // new_connection->next = clip_connections;
        // clip_connections = new_connection;

        // header.operation = OPERATION_UPDATE;

        // // send updated regions
        // for (int i = 0; i < REGIONS_QUANTITY; i++) {

        //     header.region = i;
        //     header.count = regions[i].size;

        //     memcpy(header_msg, &header, sizeof(header_t));

        //     nbytes = 0;
        //     while (nbytes < sizeof(header_t)) {
        //         nbytes += send(clip_fd, header_msg, sizeof(header_t), 0);
        //     }
        //     //send content
        //     nbytes = 0;
        //     while (nbytes < regions[i].size) {
        //         nbytes += send(clip_fd, regions[i].content, regions[i].size, 0);
        //     }
        // }
    }

    return NULL;
}

void *recv_data_clip(void *arg) {

    printf("[DEBUG][recv_data_clip] Thread code started.\n");
    // char header_msg[sizeof(header_t)];
    // header_t header;
    // size_t nbytes;
    // char *content;
    // connection_t *aux;

    // while(1) {

    //     nbytes = 0;
    //     while(nbytes < sizeof(header_t) ) {
    //         nbytes += recv(*(int *)fd, header_msg, sizeof(header_t), 0);
    //     }

    //     printf("[DEBUG] Received header from son\n");

    //     memcpy(&header, header_msg, sizeof(header_t));

    //     printf("[DEBUG] Operation: %c\n", header.operation);
    //     printf("[DEBUG] Region: %d\n", header.region);
    //     printf("[DEBUG] Length: %ld\n", header.count);

    //     if (header.operation == OPERATION_UPDATE) {

    //         if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
    //             perror("Error [malloc]");
    //             // pthread_exit(NULL);
    //         }

    //         nbytes = 0;
    //         while(nbytes < header.count) {
    //             nbytes += recv(*(int *)fd, &(content[nbytes]), header.count, 0);
    //         }

    //         // re-send header to parent
    //         if (clipboard_parent_fd != -1) {
    //             printf("[DEBUG] Sending message to parent\n");

    //             // send header
    //             nbytes = 0;
    //             while (nbytes < sizeof(header_t)) {
    //                 nbytes += send(clipboard_parent_fd, header_msg, sizeof(header_t), 0);
    //             }
    //             //send content
    //             nbytes = 0;
    //             while (nbytes < header.count) {
    //                 nbytes += send(clipboard_parent_fd, content, header.count, 0);
    //             }
    //         } else {
    //             printf("[DEBUG] I'm root!\n");
    //             if (regions[header.region].size > header.count) {
    //                 free(regions[header.region].content);
    //             }
    //             regions[header.region].size = header.count;
    //             regions[header.region].content = content;

    //             if (clip_connections != NULL) {
    //                 // send messages to other clipboards.
    //                 printf("Sending messages to leaves\n");
    //                 aux = clip_connections;
    //                 while(aux != NULL) {
    //                     //send to other clipboards
    //                     if (aux->fd > 0) {
    //                        // send header
    //                         nbytes = 0;
    //                         while (nbytes < sizeof(header_t)) {
    //                             nbytes += send(aux->fd, header_msg, sizeof(header_t), 0);
    //                         }
    //                         //send content
    //                         nbytes = 0;
    //                         while (nbytes < header.count) {
    //                             nbytes += send(aux->fd, content, header.count, 0);
    //                         }
    //                     }
    //                     aux = aux->next;
    //                 }
    //             }
    //         }

    //     } else {
    //         printf("[DEBUG] Invalid operation\n");
    //     }


    // }
    return NULL;
}

void *recv_data_parent_clip(void *arg) {

    printf("[DEBUG][recv_data_parent_clip] Thread code started.\n");
    // printf("[DEBUG] New thread created (tcp client comm)\n");
    // size_t nbytes;
    // char header_msg[sizeof(header_t)];
    // header_t header;
    // char *content;
    // connection_t *aux;

    // while(1) {
    //     nbytes = recv(*(int *)fd, header_msg, sizeof(header_t), 0);
    //     printf("[DEBUG] Received header from parent clipboard\n");
    //     if (nbytes == -1 || nbytes == 0) {
    //         printf("Error reading from clipboard or clipboard closed!\n");
    //         pthread_exit(NULL);
    //     } else {
    //         printf("[DEBUG] Received header (%ld bytes) from clipboard.\n", nbytes);
    //     }

    //     memcpy(&header, header_msg, sizeof(header_t));
    //     printf("[DEBUG] Operation: %c\n", header.operation);
    //     printf("[DEBUG] Region: %d\n", header.region);
    //     printf("[DEBUG] Length: %ld\n", header.count);

    //     if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
    //         perror("Error [malloc]");
    //     }

    //     nbytes = 0;
    //     while(nbytes < header.count) {
    //         nbytes += recv(*(int *)fd, &(content[nbytes]), header.count, 0);
    //     }
    //     printf("[DEBUG] Received %ld bytes\n", nbytes);
    //     printf("[DEBUG] UPDATE!!!!!\n");

    //     if (regions[header.region].size > header.count) {
    //         free(regions[header.region].content);
    //     }
    //     regions[header.region].size = header.count;
    //     regions[header.region].content = content;

    //     // send to sons
    //     if (clip_connections != NULL) {
    //         // send messages to other clipboards.
    //         printf("Sending messages to leaves\n");
    //         aux = clip_connections;
    //         while(aux != NULL) {
    //             //send to other clipboards
    //             if (aux->fd > 0) {
    //                 // send header
    //                 nbytes = 0;
    //                 while (nbytes < sizeof(header_t)) {
    //                     nbytes += send(aux->fd, header_msg, sizeof(header_t), 0);
    //                 }
    //                 //send content
    //                 nbytes = 0;
    //                 while (nbytes < header.count) {
    //                     nbytes += send(aux->fd, content, header.count, 0);
    //                 }
    //             }
    //             aux = aux->next;
    //         }
    //     }
    // }
    return NULL;
}