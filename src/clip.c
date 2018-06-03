#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#include "communication.h"
#include "protocol.h"
#include "threads.h"

#define CLIP_SOCK_NAME "CLIPBOARD_SOCKET"

void sign_interrupt_handler(int signum);

// Only set by SIGINT handler
int shutdown_clipboard = 0;

void sign_interrupt_handler(int signum) {
    // printf("[DEBUG] Caught signal Ctrl-C\n");
    shutdown_clipboard = 1;
}

int main(int argc, char **argv) {
    // Usage: clipboard [-c ip port]
    struct sockaddr_un sv_app_addr;
    struct sockaddr_in sv_clip_addr, parent_clip_addr;
    int app_ls_fd, clip_ls_fd;
    int parent_clip_fd = -1; // if -1, it's the root
    int sv_clip_port;
    pthread_t recv_conn_app_id, recv_conn_clip_id, recv_data_parent_clip_id;
    recv_conn_app_args_t recv_conn_app_args;
    recv_conn_clip_args_t recv_conn_clip_args;
    recv_data_parent_clip_args_t recv_data_parent_clip_args;
    data_region_t regions[REGIONS_QUANTITY];
    connection_t *children_connections = NULL;
    pthread_rwlock_t *region_locks[10];
    char opt;
    connection_t *aux;

    // Handle Ctrl-C interrupt
	signal(SIGINT, sign_interrupt_handler);

    // check for correct number of arguments
    if (argc != 1 && argc != 4) {
        printf("Usage: clipboard [-c ip port]\n");
        exit(-1);
    }

    // Initialize regions
    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        regions[i].size = 0;
        regions[i].content = NULL;
    }

    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        region_locks[i] = (pthread_rwlock_t *)malloc(1*sizeof(pthread_rwlock_t));
    }

    if (init_stream_unix_sv(&app_ls_fd, &sv_app_addr, CLIP_SOCK_NAME) == -1) {
        printf("Error initializing unix stream server.\n");
        exit(-1);
    }

    recv_conn_app_args.fd = app_ls_fd;
    recv_conn_app_args.parent_clip_fd = &parent_clip_fd;
    recv_conn_app_args.regions = regions;
    recv_conn_app_args.children_clip_connections = &children_connections;
    recv_conn_app_args.region_locks = region_locks;
    if (pthread_create(&recv_conn_app_id, NULL, recv_conn_app, &recv_conn_app_args) != 0) {
        perror("Error [pthread_create]");
        exit(-1);
    }

    if (init_tcp_sv(&clip_ls_fd, &sv_clip_addr, 0) == -1) {
        printf("Error initializing TCP server.\n");
        exit(-1);
    }

    // print port to terminal
    if (tcp_get_port(clip_ls_fd, &sv_clip_port) == -1) {
        printf("Error getting TCP server port.\n");
        exit(-1);
    }
    printf("TCP server port: %d\n", sv_clip_port);

    recv_conn_clip_args.fd = clip_ls_fd;
    recv_conn_clip_args.children_connections = &children_connections;
    recv_conn_clip_args.regions = regions;
    recv_conn_clip_args.parent_clip_fd = &parent_clip_fd;
    recv_conn_clip_args.region_locks = region_locks;
    if (pthread_create(&recv_conn_clip_id, NULL, recv_conn_clip, &recv_conn_clip_args) != 0) {
        perror("Error [pthread_create]");
        exit(-1);
    }

    // Connected mode: Create tcp client and connect to remote clipboard
    if (argc == 4 && strcmp(argv[1], "-c") == 0) {

        if (init_tcp_client(&parent_clip_fd, &parent_clip_addr, argv[2], atoi(argv[3])) == -1) {
            printf("Error initializing TCP client.\n");
            exit(-1);
        }
        recv_data_parent_clip_args.fd = parent_clip_fd;
        recv_data_parent_clip_args.regions = regions;
        recv_data_parent_clip_args.children_clip_connections = &children_connections;
        recv_data_parent_clip_args.region_locks = region_locks;
        recv_data_parent_clip_args.parent_clip_fd = &parent_clip_fd;
        if (pthread_create(&recv_data_parent_clip_id, NULL, recv_data_parent_clip, &recv_data_parent_clip_args) != 0) {
            perror("Error [pthread_create]");
            exit(-1);
        }
    }

    while(!shutdown_clipboard) {

        opt = getchar();

        if (opt == 'r') {
            printf("Regions information:\n");
            for (int i = 0; i < REGIONS_QUANTITY; i++) {
                printf("Region: %d | size: %ld | content: ", i, regions[i].size);
                regions[i].content == NULL ? printf("NULL\n") : printf("%.*s\n", (int)regions[i].size, (char *)regions[i].content);
            }
        } else if (opt == 'q') {
            shutdown_clipboard = 1;
        }
    }

    printf("Shuting down clipboard\n");

    // TODO: Clean all memory
    // close UNIX socket
    if (close_stream_unix_sv(CLIP_SOCK_NAME) != 0) {
        printf("Error closing UNIX socket.\n");
    }

    // close tcp server
    tcp_close(clip_ls_fd);

    // Connected mode: Close tcp client
    if (argc == 4 && strcmp(argv[1], "-c") == 0) {
        if (parent_clip_fd != -1) {
            tcp_close(parent_clip_fd);
        }
    }

    // Clean all memory
    // clean regions
    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        free(regions[i].content);
    }

    // clean locks
    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        free(region_locks[i]);
    }

    // // clean children list
    while(children_connections != NULL) {
        aux = children_connections;
        children_connections = aux->next;
        free(aux);
    }
    return 0;
}