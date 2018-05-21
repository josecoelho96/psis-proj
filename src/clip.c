#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "clipboard.h"

#define REGIONS_QUANTITY 10

void *thr_code_recv_conn_loc_app(void *fd);
void *thr_code_recv_conn_app(void *fd);

void *thr_code_recv_conn_loc_app(void *fd) {

    int app_fd = 0;
    struct sockaddr_un app_addr;
    pthread_t thr_id_conn_app;
    socklen_t addrlen = sizeof(app_addr);

    printf("[DEBUG] New thread created (app listening thread)\n");
    while(1) {

        if ((app_fd = accept(*(int *)fd, (struct sockaddr*)&app_addr, &addrlen)) == -1) {
            perror("Error [accept ls socket]");
            pthread_exit(NULL);
        }
        printf("[DEBUG] [App listening thread] New app connecting...\n");

        // Create new thread for the newly connected app
        if (pthread_create(&thr_id_conn_app, NULL, thr_code_recv_conn_app, &app_fd) != 0) {
            perror("Error [pthread_create]");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void *thr_code_recv_conn_app(void *fd) {
    int buf_len = 10;
    char buf[buf_len];
    int nbytes;
    char *message = NULL;
    char dest[8];
    int msg_size;

    printf("[DEBUG] New thread created (communication thread)\n");
    while(1) {
        printf("[DEBUG][App thread] Got a connection with an app...\n");

        nbytes = recv(*(int *)fd, buf, buf_len, 0);
        printf("[DEBUG][App thread] After read\n");
        if (nbytes == -1 || nbytes == 0) {
            printf("Error reading from app or app closed!\n");
            pthread_exit(NULL);
        } else {
            printf("[DEBUG] Received '%s' (%d bytes) from app.\n", buf, nbytes);
        }

        printf("Received: %s\n", buf);

        strncpy(dest, &buf[2], 8);
        printf("Operation: %c\n", buf[0]);
        printf("Region: %c\n", buf[1]);
        printf("Length: %d\n", atoi(dest));
        msg_size = atoi(dest) + 1;
        if ((message = (char *)malloc(msg_size * sizeof(char))) == NULL) {
            perror("Error [malloc]");
            pthread_exit(NULL);
        }

        if (buf[0] == 'c') {
            nbytes = recv(*(int *)fd, message, msg_size, 0);
            printf("Received: %s\n", message);
            // TODO: save on data_regions
        } else if (buf[0] == 'p') {
            printf("Operation: paste\n");
            nbytes = recv(*(int *)fd, message, msg_size, 0);
        } else if (buf[0] == 'w') {
            printf("Operation: wait\n");
        } else {
            printf("Wrong operation!\n");
            continue;
        }
    }
    return NULL;
}


int main(int argc, char **argv) {
    // Usage: clipboard [-c ip port]

    struct sockaddr_un clipboard_addr;
    int clipboard_ls_fd;
    data_region *regions = NULL;
    pthread_t thr_id_recv_conn_loc_app;

    // check for correct number of arguments
    if (argc != 1 && argc != 4) {
        printf("Usage: clipboard [-c ip port]\n");
        exit(-1);
    }

    printf("[DEBUG] Correct number of arguments.\n");

    // Initialize regions
    if ((regions = (data_region*)malloc(REGIONS_QUANTITY * sizeof(data_region))) == NULL) {
        perror("Error [malloc]");
        exit(-1);
    }
    for (int i = 0; i < REGIONS_QUANTITY; i++) {
        regions[i].size = 0;
        regions[i].content = NULL;
    }

    printf("[DEBUG] Regions initialized.\n");

    if ((clipboard_ls_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        exit(-1);
    }

    printf("[DEBUG] socket created.\n");

    memset((void*)&clipboard_addr, (int)'\0', sizeof(clipboard_addr));
    clipboard_addr.sun_family = AF_UNIX;
    strcpy(clipboard_addr.sun_path, "CLIPBOARD_SOCKET");

    if (bind(clipboard_ls_fd, (struct sockaddr *)&clipboard_addr, sizeof(clipboard_addr)) == -1) {
        perror("Error [bind]");
        exit(-1);
    }

    printf("[DEBUG] socket binded.\n");

    if(listen(clipboard_ls_fd, 5) == -1) {
        perror("Error [listen]");
        exit(-1);
    }

    printf("[DEBUG] Ready to listen.\n");

    if (pthread_create(&thr_id_recv_conn_loc_app, NULL, thr_code_recv_conn_loc_app, &clipboard_ls_fd) != 0) {
        perror("Error [pthread_create]");
        exit(-1);
    }

    // logic goes here
    while(1) {
        printf("[DEBUG] [Clipboard main thread] Loop\n");
        sleep(1);
    }

    // close UNIX socket
    if (unlink("CLIPBOARD_SOCKET") == -1) {
        perror("Error [unlink]");
        exit(-1);
    }

    return 0;
}