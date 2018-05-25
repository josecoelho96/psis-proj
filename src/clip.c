#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "clipboard.h"

typedef struct d_region {
    void *content;
    size_t size;
} data_region;

typedef struct connection {
  int fd;
  struct connection *next;
} connection_t;

#define REGIONS_QUANTITY 10
data_region regions[REGIONS_QUANTITY];
connection_t *clip_connections = NULL;

void ctrl_c_callback_handler(int signum);
void *thr_code_recv_conn_app(void *fd);
void *thr_code_recv_data_app(void *fd);
void *thr_code_recv_conn_rem_clip(void *fd);
void *thr_code_recv_data_clip(void *fd);
void *thr_code_recv_data_clip_client(void *fd);

void *thr_code_recv_conn_app(void *fd) {

    int app_fd;
    struct sockaddr_un app_addr;
    pthread_t thr_id_conn_app;
    socklen_t addrlen = sizeof(app_addr);

    // printf("[DEBUG] New thread created (app listening thread)\n");
    while(1) {

        if ((app_fd = accept(*(int *)fd, (struct sockaddr*)&app_addr, &addrlen)) == -1) {
            perror("Error [accept ls socket]");
            pthread_exit(NULL);
        }
        // printf("[DEBUG] [App listening thread] New app connecting...\n");

        // Create new thread for the newly connected app
        // TODO: all pthread_t should be saved (?)
        if (pthread_create(&thr_id_conn_app, NULL, thr_code_recv_data_app, &app_fd) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void *thr_code_recv_data_app(void *fd) {

    size_t nbytes;
    char header_msg[sizeof(header_t)];
    header_t header;
    char *content = NULL;

    // int buf_len = 10;
    // char buf[buf_len];

    // size_t bytes_paste;

    // char dest[8];
    // int content_size;
    // int region;
    // char header_str[10];
    // connection_t *aux;

    // printf("[DEBUG] New thread created (communication thread)\n");
    // printf("[DEBUG][App thread] Got a connection with an app...\n");
    while(1) {

        nbytes = recv(*(int *)fd, header_msg, sizeof(header_t), 0);
        if (nbytes == -1 || nbytes == 0) {
            printf("Error reading from app or app closed!\n");
            pthread_exit(NULL);
        } else {
            printf("[DEBUG] Received header (%ld bytes) from app.\n", nbytes);
        }
        memcpy(&header, header_msg, sizeof(header_t));

        printf("[DEBUG] Operation: %c\n", header.operation);
        printf("[DEBUG] Region: %d\n", header.region);
        printf("[DEBUG] Length: %ld\n", header.count);

        if (header.operation == OPERATION_COPY) {
            printf("[DEBUG] Operation: Copy\n");

            if ((content = (char *)malloc(header.count * sizeof(char))) == NULL) {
                perror("Error [malloc]");
                // pthread_exit(NULL);
            }
            nbytes = 0;
            while(nbytes < header.count) {
                nbytes += recv(*(int *)fd, &(content[nbytes]), header.count, 0);
            }

            printf("[DEBUG] Received %ld bytes\n", nbytes);
            // TODO: DEBUG: HERE!!!!
            // // TODO: Only free if new content is bigger
            // regions[region].size = content_size;
            // free(regions[region].content);
            // regions[region].content = content;

            // if (clip_connections != NULL) {
            //     // send messages to other clipboards.
            //     printf("Sending messages is a must\n");
            //     buf[0] = 'u'; //change to update
            //     aux = clip_connections;
            //     while(aux!=NULL) {
            //         //send to other clipboards
            //         printf("Sending %.*s to: %d\n", 10, buf, aux->fd);
            //         if (aux->fd > 0) {
            //             send(aux->fd, buf, 10, 0);
            //         }
            //         aux = aux->next;
            //     }
            // }

        } else if (header.operation == OPERATION_PASTE) {
            printf("[DEBUG] Operation: Paste\n");

            // printf("[DEBUG] Paste %d bytes (max) from region %d\n", content_size, region);
            // printf("[DEBUG] Actual region data size: %ld\n", regions[region].size);

            // bytes_paste = regions[region].size > content_size ? content_size : regions[region].size;
            // printf("[DEBUG] Actual paste: %ld\n", bytes_paste);
            // sprintf(header, "p%ld", bytes_paste);
            // send(*(int *)fd, header, 9, 0);
            // send(*(int *)fd, regions[region].content, bytes_paste, 0);
        } else if (header.operation == OPERATION_WAIT) {
            printf("[DEBUG][TODO:] Operation: Wait\n");
        } else {
            printf("Invalid operation!\n");
            continue;
        }
    }
    return NULL;
}

void *thr_code_recv_conn_rem_clip(void *fd) {

    int clip_fd = 0;
    struct sockaddr_in clip_addr;
    pthread_t thr_id_conn_clip;
    socklen_t addrlen = sizeof(clip_addr);
    connection_t *new_connection = NULL;


    printf("[DEBUG] New thread created (clipboard listening thread)\n");
    while(1) {

        if ((clip_fd = accept(*(int *)fd, (struct sockaddr*)&clip_addr, &addrlen)) == -1) {
            perror("Error [accept ls socket]");
            pthread_exit(NULL);
        }

        printf("[DEBUG] [Clipboard listening thread] New clipboard connecting (fd: %d)\n", clip_fd);


        if ((new_connection = (connection_t*)malloc(sizeof(connection_t))) == NULL) {
            perror("Error [malloc]");
            pthread_exit(NULL);
        };

        // add new connection to list
        // TODO: Improve: Check if some node is invalid.
        new_connection->fd = clip_fd;
        new_connection->next = clip_connections;
        clip_connections = new_connection;

        // Create new thread for the newly connected app
        // TODO: all pthread_t should be saved (?)
        if (pthread_create(&thr_id_conn_clip, NULL, thr_code_recv_data_clip, &clip_fd) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void *thr_code_recv_data_clip(void *fd) {
    int buf_len = 10;
    char buf[buf_len];
    int nbytes;
    char dest[8];
    int content_size;
    int region = 0;

    printf("[DEBUG][Clipboard thread] Got a connection with an clipboard...\n");

    while(1) {
        nbytes = recv(*(int *)fd, buf, buf_len, 0);
        printf("[DEBUG] Received header from other clipboard (on TCP server).\n");
        if (nbytes == -1 || nbytes == 0) {
            printf("Error reading from clipboard or clipboard closed!\n");
            pthread_exit(NULL);
        } else {
            printf("[DEBUG] Received header '%.*s' (%d bytes) from clipboard.\n", buf_len, buf, nbytes);
        }

        strncpy(dest, &buf[2], 8);
        content_size = atoi(dest);
        region = buf[1] - '0';

        printf("Reading %d bytes\n", content_size);

    }
    return NULL;
}

void *thr_code_recv_data_clip_client(void *fd) {

    printf("[DEBUG] New thread created (tcp client comm)\n");
    int nbytes;
    int buf_len = 10;
    char buf[buf_len];

    while(1) {
        nbytes = recv(*(int *)fd, buf, buf_len, 0);
        printf("[DEBUG] Received header from other clipboard (on TCP client).\n");
        if (nbytes == -1 || nbytes == 0) {
            printf("Error reading from clipboard or clipboard closed!\n");
            pthread_exit(NULL);
        } else {
            printf("[DEBUG] Received header '%s' (%d bytes) from clipboard.\n", buf, nbytes);
        }
    }
    return NULL;
}

void ctrl_c_callback_handler(int signum) {
	printf("Caught signal Ctr-C\n");
	unlink("CLIPBOARD_SOCKET");
	exit(0);
}

int main(int argc, char **argv) {
    // Usage: clipboard [-c ip port]

    struct sockaddr_un clipboard_addr;
    struct sockaddr_in remote_clipboard_addr;
    struct sockaddr_in client_clipboard_addr;
    socklen_t remote_clipboard_addr_len;
    int app_ls_fd;
    int clipboard_ls_fd;
    int client_clipboard_fd;
    pthread_t thr_id_recv_conn_loc_app;
    pthread_t thr_id_recv_conn_rem_clip;
    pthread_t thr_id_recv_client_clip;

	signal(SIGINT, ctrl_c_callback_handler);

    // check for correct number of arguments
    if (argc != 1 && argc != 4) {
        printf("Usage: clipboard [-c ip port]\n");
        exit(-1);
    }

    // printf("[DEBUG] Correct number of arguments.\n");

    // Initialize regions
    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        regions[i].size = 0;
        regions[i].content = NULL;
    }

    // printf("[DEBUG] Regions initialized.\n");

    if ((app_ls_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        exit(-1);
    }
    // printf("[DEBUG] socket created.\n");
    memset((void*)&clipboard_addr, (int)'\0', sizeof(clipboard_addr));
    clipboard_addr.sun_family = AF_UNIX;
    strcpy(clipboard_addr.sun_path, "CLIPBOARD_SOCKET");
    if (bind(app_ls_fd, (struct sockaddr *)&clipboard_addr, sizeof(clipboard_addr)) == -1) {
        perror("Error [bind]");
        exit(-1);
    }
    // printf("[DEBUG] socket binded.\n");
    if(listen(app_ls_fd, 5) == -1) {
        perror("Error [listen]");
        exit(-1);
    }

    // printf("[DEBUG] Ready to listen.\n");

    if (pthread_create(&thr_id_recv_conn_loc_app, NULL, thr_code_recv_conn_app, &app_ls_fd) != 0) {
        perror("Error [pthread_create]");
        exit(-1);
    }

    if ((clipboard_ls_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        exit(-1);
    }
    memset((void*)&remote_clipboard_addr, (int)'\0', sizeof(remote_clipboard_addr));
    remote_clipboard_addr.sin_family = AF_INET;
    remote_clipboard_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    remote_clipboard_addr.sin_port = htons(0);

    if (bind(clipboard_ls_fd, (struct sockaddr*)&remote_clipboard_addr, sizeof(remote_clipboard_addr)) == -1) {
        perror("Error [bind]");
        exit(-1);
    }

    if (listen(clipboard_ls_fd, 5) == -1) {
        perror("Error [listen]");
        exit(-1);
    }

    // print port to terminal
    remote_clipboard_addr_len = sizeof(remote_clipboard_addr);
    if (getsockname(clipboard_ls_fd, (struct sockaddr *)&remote_clipboard_addr, &remote_clipboard_addr_len) == -1) {
        perror("Error [getsockname]");
        exit(-1);
    } else {
        printf("TCP server port: %d\n", ntohs(remote_clipboard_addr.sin_port));
    }

    if (pthread_create(&thr_id_recv_conn_rem_clip, NULL, thr_code_recv_conn_rem_clip, &clipboard_ls_fd) != 0) {
        perror("Error [pthread_create]");
        exit(-1);
    }

    // if on connected mode, create tcp client and connect to remote clipboard
    if (argc == 4) {
        printf("[DEBUG] Connect!\n");
        if ((client_clipboard_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Error [socket]");
            exit(-1);
        }

        memset((void*)&client_clipboard_addr, (int)'\0', sizeof(client_clipboard_addr));
        if (inet_pton(AF_INET, argv[2], &(client_clipboard_addr.sin_addr)) != 1) {
            perror("Error [inet_pton]");
            exit(-1);
        }

        client_clipboard_addr.sin_family = AF_INET;
        client_clipboard_addr.sin_port = htons(atoi(argv[3]));

        if (connect(client_clipboard_fd, (struct sockaddr*)&client_clipboard_addr, sizeof(client_clipboard_addr)) != 0) {
            perror("Error [connect]");
            exit(-1);
        }

        if (pthread_create(&thr_id_recv_client_clip, NULL, thr_code_recv_data_clip_client, &client_clipboard_fd) != 0) {
           perror("Error [pthread_create]");
            exit(-1);
        }
    }

    // logic goes here
    while(1) {

        // printf("[DEBUG] [Clipboard main thread] Regions information\n");
        // for (int i = 0; i < REGIONS_QUANTITY; i++) {
        //     printf("Region: %d | size: %ld | content: ", i, regions[i].size);
        //     regions[i].content == NULL ? printf("NULL\n") : printf("%.*s\n", (int)regions[i].size, (char *)regions[i].content);
        // }

        sleep(1);
    }

    // close UNIX socket
    if (unlink("CLIPBOARD_SOCKET") == -1) {
        perror("Error [unlink]");
        exit(-1);
    }

    return 0;
}