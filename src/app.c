#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clipboard.h"

int main(int argc, char **argv) {

    int fd;
    int cmdline_len = 100;
    char cmdline_buf[100];

    if ((fd = clipboard_connect(argv[1])) == -1) {
        exit(-1);
    }

    while(1) {
        fgets(cmdline_buf, cmdline_len, stdin);
        if (strcmp(cmdline_buf, "copy\n") == 0) {

            printf("Copy data to clipboard\n");
            char message[]="Hello World!\n";
            int msg_len = strlen(message);
            printf("Copied %d bytes\n", clipboard_copy(fd, 0, message, msg_len+1));


        } else if (strcmp(cmdline_buf, "paste\n") == 0) {
            printf("TODO: Paste\n");
        } else if (strcmp(cmdline_buf, "wait\n") == 0) {
            printf("TODO: Wait\n");
        } else if (strcmp(cmdline_buf, "close\n") == 0) {
            printf("TODO: Close\n");
        } else if (strcmp(cmdline_buf, "connect\n") == 0) {
            printf("TODO: Connect\n");
        } else {
            printf("Valid options:\n- copy\n- paste\n- wait\n- connect\n- close\n");
        }
    }

   return 0;
}