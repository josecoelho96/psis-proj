#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>

#include "lib/com.h"

int main(int argc, char **argv) {
    // Usage: clipboard [-c ip port]

    struct sockaddr_un clipboard_addr;
    int clipboard_fd;

    // check for correct number of arguments
    if (argc != 1 && argc != 4) {
        printf("Usage: clipboard [-c ip port]\n");
        exit(-1);
    }

    // create UNIX socket to communicate with app
    if (unix_stream_init_server(&clipboard_fd, &clipboard_addr, "CLIPBOARD_SOCKET") == -1) {
        exit(-1);
    }

    // logic goes here

    // close UNIX socket
    if (unix_stream_close("CLIPBOARD_SOCKET") == -1) {
        exit(-1);
    }

    return 0;
}