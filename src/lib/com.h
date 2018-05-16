#ifndef COM_H
#define COM_H

int unix_stream_init_server(int *fd, struct sockaddr_un *addr, char *dir);
int unix_stream_close(char *dir);

#endif