#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <sys/un.h>
#include <arpa/inet.h>

int init_stream_unix_sv(int *fd, struct sockaddr_un *addr, char *dir);
int init_stream_unix(int *fd, struct sockaddr_un *addr, char *dir);
int stream_unix_accept(int fd, struct sockaddr_un *addr);
int close_stream_unix(char *dir);
int init_tcp_sv(int *fd, struct sockaddr_in *addr, int port);
int init_tcp_client(int *fd, struct sockaddr_in *addr, char *ip, int port);
int tcp_accept(int fd, struct sockaddr_in *addr);
int tcp_get_port(int fd, int *port);
int recv_data(int fd, char *buf, size_t count);
int send_data(int fd, char *buf, size_t count);
#endif