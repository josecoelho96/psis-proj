#ifndef COM_H
#define COM_H

int unix_stream_read(int fd, void *buf, int buf_len);
int unix_stream_write(int fd, void *buf, int buf_len);

#endif