#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>

#define OPERATION_COPY 'C'
#define OPERATION_PASTE 'P'
#define OPERATION_WAIT 'W'
#define OPERATION_UPDATE 'U'
#define REGIONS_QUANTITY 10

typedef struct header {
    char operation;
    int region;
    size_t count;
} header_t;

typedef struct data_region {
    void *content;
    size_t size;
} data_region_t;

typedef struct connection {
  int fd;
  struct connection *next;
} connection_t;

int recv_header(int fd, header_t *header);
int recv_content(int fd, header_t header, data_region_t *regions);
int send_header(int fd, header_t header);
int send_content(int fd, header_t header, data_region_t *regions);
int send_message(int fd, header_t header, data_region_t *regions);
int send_regions(int fd, data_region_t *regions);
int update_children(connection_t *head, header_t header, data_region_t *regions);
#endif