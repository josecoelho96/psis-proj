#include <sys/types.h>

#define OPERATION_COPY 'C'
#define OPERATION_PASTE 'P'
#define OPERATION_WAIT 'W'
#define OPERATION_UPDATE 'U'

typedef struct header {
    char operation;
    int region;
    size_t count;
} header_t;

int clipboard_connect(char *clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
