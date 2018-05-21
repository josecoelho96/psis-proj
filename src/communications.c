
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "communications.h"


int unix_stream_read(int fd, void *buf, int buf_len) {

    // TODO: will not work for extra long buffers
    int n_read = 0;
    while ((n_read = recv(fd, buf, buf_len, 0)) > 0);
    printf("Data read\n");
    return n_read;
}

int unix_stream_write(int fd, void *buf, int buf_len) {

    // TODO: will not work for extra long buffers
    return write(fd, buf, buf_len);
}