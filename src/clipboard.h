#include <sys/types.h>

typedef struct d_region {
    void *content;
    int size;
} data_region;

int clipboard_connect(char *clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
