#ifndef THREADS_H
#define THREADS_H

typedef struct recv_conn_app_args {
    int fd;
    int *parent_clip_fd;
    void *children_clip_connections;
    void *regions;
    void *region_locks;
} recv_conn_app_args_t;

typedef struct recv_data_app_args {
    int fd;
    int *parent_clip_fd;
    void *children_clip_connections;
    void *regions;
    void *region_locks;
} recv_data_app_args_t;

typedef struct recv_conn_clip_args {
    int fd;
    void *children_connections;
    int *parent_clip_fd;
    void *regions;
    void *region_locks;
} recv_conn_clip_args_t;

typedef struct recv_data_clip_args {
    int fd;
    void *regions;
    void *children_connections;
    int *parent_clip_fd;
    void *region_locks;
} recv_data_clip_args_t;

typedef struct recv_data_parent_clip_args {
    int fd;
    void *regions;
    void *children_clip_connections;
    void *region_locks;
    int *parent_clip_fd;
} recv_data_parent_clip_args_t;

void *recv_conn_app(void *arg);
void *recv_data_app(void *arg);
void *recv_conn_clip(void *arg);
void *recv_data_clip(void *arg);
void *recv_data_parent_clip(void *arg);

#endif