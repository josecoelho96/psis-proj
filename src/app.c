#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clipboard.h"

int main(int argc, char **argv) {

    int fd;
    int cmdline_len = 100;
    char cmdline_buf[100];
    char *buf;
    int recv_buf_len = 100;
    char *recv_buf;
    char *new_dir;
    int region_number;
    int data_length;
    int bytes_sent;
    char c;

    if ((fd = clipboard_connect(argv[1])) == -1) {
        exit(-1);
    }

    new_dir = (char *)malloc(108*sizeof(char));

    while(1) {
        fgets(cmdline_buf, cmdline_len, stdin);
        if (strcmp(cmdline_buf, "copy\n") == 0) {
            printf("Copy data to clipboard...\n");
            printf("Region: ");
            fgets(cmdline_buf, cmdline_len, stdin);
            if (sscanf(cmdline_buf, "%d", &region_number) != 1) {
                printf("Invalid region\n");
                continue;
            }
            printf("Data length: ");
            fgets(cmdline_buf, cmdline_len, stdin);
            if (sscanf(cmdline_buf, "%d", &data_length) != 1 || data_length <= 0) {
                printf("Invalid data size\n");
                continue;
            }

            if ((buf = (char *)malloc(data_length * sizeof(char))) == NULL) {
                printf("Error allocating memory\n");
                continue;
            }

            printf("Character: ");
            c = getchar();

            for (int i = 0; i < data_length; i++) {
                buf[i] = c;
            }

            printf("Sending %d bytes...\n", data_length);

            if ((bytes_sent = clipboard_copy(fd, region_number, buf, data_length)) == 0) {
                printf("Error copying to clipboard.\n");
            } else {
                printf("Copied %d bytes\n", bytes_sent);
            }

        } else if (strcmp(cmdline_buf, "paste\n") == 0) {
            printf("Paste data from clipboard...\n");
            printf("Region: ");
            fgets(cmdline_buf, cmdline_len, stdin);
            if (sscanf(cmdline_buf, "%d", &region_number) != 1) {
                printf("Invalid region\n");
                continue;
            }
            printf("Data length (max): ");
            fgets(cmdline_buf, cmdline_len, stdin);
            if (sscanf(cmdline_buf, "%d", &recv_buf_len) != 1 || recv_buf_len < 0) {
                printf("Invalid data length\n");
                continue;
            }

            if (recv_buf != NULL) free(recv_buf);
            recv_buf = (char *)malloc(recv_buf_len * sizeof(char));
            clipboard_paste(fd, region_number, recv_buf, recv_buf_len);
            printf("Received data: %.*s\n", recv_buf_len, recv_buf);
        } else if (strcmp(cmdline_buf, "wait\n") == 0) {
            printf("TODO: Wait\n");
        } else if (strcmp(cmdline_buf, "close\n") == 0) {
            printf("Closing connection to clipboard ...\n");
            clipboard_close(fd);
        } else if (strcmp(cmdline_buf, "connect\n") == 0) {
            printf("Connecting to clipboard...\n");
            printf("Directory: ");
            fgets(cmdline_buf, cmdline_len, stdin);
            memcpy(new_dir, cmdline_buf, strlen(cmdline_buf)-1);
            printf("Connecting to: %s\n", cmdline_buf);
            if ((fd = clipboard_connect(new_dir)) == -1) {
                exit(-1);
            }
        } else {
            printf("Valid options:\n- copy\n- paste\n- wait\n- connect\n- close\n");
        }
    }

    free(new_dir);
   return 0;
}