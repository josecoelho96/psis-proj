#ifndef THREADS_H
#define THREADS_H

typedef struct recv_conn_app_args {
    int fd;
    int *parent_clip_fd;
    void *regions;
} recv_conn_app_args_t;

typedef struct recv_data_app_args {
    int fd;
    int *parent_clip_fd;
    void *regions;
} recv_data_app_args_t;

typedef struct recv_conn_clip_args {
    int fd;
} recv_conn_clip_args_t;

typedef struct recv_data_clip_args {
    int fd;
} recv_data_clip_args_t;

typedef struct recv_data_parent_clip_args {
    int fd;
} recv_data_parent_clip_args_t;

void *recv_conn_app(void *arg);
void *recv_data_app(void *arg);
void *recv_conn_clip(void *arg);
void *recv_data_clip(void *arg);
void *recv_data_parent_clip(void *arg);

#endif