#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "communication.h"

int init_stream_unix_sv(int *fd, struct sockaddr_un *addr, char *dir) {

    if ((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        return -1;
    }

    memset((void*)addr, (int)'\0', sizeof(*addr));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, dir);

    if (bind(*fd, (struct sockaddr *)addr, sizeof(*addr)) == -1) {
        perror("Error [bind]");
        return -1;
    }

    if (listen(*fd, 5) == -1) {
        perror("Error [listen]");
        return -1;
    }

    return 0;
}

int init_stream_unix(int *fd, struct sockaddr_un *addr, char *dir) {

    if ((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        return -1;
    }
    memset((void*)addr, (int)'\0', sizeof(*addr));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, dir);

	if (connect(*fd, (const struct sockaddr *)addr, sizeof(*addr)) == -1) {
        perror("Error [connect]");
        return -1;
	}
    return 0;
}

int stream_unix_accept(int fd, struct sockaddr_un *addr) {
    int new_fd;

    socklen_t addrlen = sizeof(*addr);
    if ((new_fd = accept(fd, (struct sockaddr*)addr, &addrlen)) == -1) {
        perror("Error [accept]");
        return -1;
    }
    return new_fd;
}

int stream_unix_recv(int fd, char *buf, size_t count) {

    size_t bytes_recv = 0;
    size_t total_bytes_recv = 0;

    while(total_bytes_recv < count) {
        bytes_recv = recv(fd, buf, count, 0);
        if (bytes_recv == -1) {
            printf("Error reading!\n");
            return -1;
        } else if (bytes_recv == 0) {
            printf("Peer has shutdown!\n");
            return 0;
        }
        total_bytes_recv += bytes_recv;
    }
    return 1;
}

int stream_unix_send(int fd, char *buf, size_t count) {

    size_t bytes_sent = 0;
    size_t total_bytes_sent = 0;

    while(total_bytes_sent < count) {
        bytes_sent = send(fd, buf, count, 0);
        if (bytes_sent == -1) {
            printf("Error sending!\n");
            return -1;
        } else if (bytes_sent == 0) {
            printf("Peer has shutdown!\n");
            return 0;
        }
        total_bytes_sent += bytes_sent;
    }
    return 1;
}

int init_tcp_sv(int *fd, struct sockaddr_in *addr, int port) {

    if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);


    if (bind(*fd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
        perror("Error [bind]");
        return -1;
    }

    if (listen(*fd, 5) == -1) {
        perror("Error [listen]");
        return -1;
    }

    return 0;
}

int init_tcp_client(int *fd, struct sockaddr_in *addr, char *ip, int port) {

    if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error [socket]");
        return -1;
    }

    memset((void*)addr, (int)'\0', sizeof(*addr));

    if (inet_pton(AF_INET, ip, &(addr->sin_addr)) != 1) {
        perror("Error [inet_pton]");
        return -1;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    if (connect(*fd, (struct sockaddr*)addr, sizeof(*addr)) != 0) {
        perror("Error [connect]");
        return -1;
    }
    return 0;
}

int tcp_get_port(int fd, int *port) {

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getsockname(fd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("Error [getsockname]");
        return -1;
    } else {
        *port = ntohs(addr.sin_port);
        return 0;
    }
}

int close_stream_unix(char *dir) {

    if (unlink(dir) == -1) {
        perror("Error [unlink]");
        return -1;
    }
    return 0;
}

int tcp_accept(int fd, struct sockaddr_in *addr) {
    int new_fd;

    socklen_t addrlen = sizeof(*addr);
    if ((new_fd = accept(fd, (struct sockaddr*)addr, &addrlen)) == -1) {
        perror("Error [accept]");
        return -1;
    }
    return new_fd;
}