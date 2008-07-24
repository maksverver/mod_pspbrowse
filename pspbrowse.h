#ifndef PSPBROWSE_INCLUDED
#define PSPBROWSE_INCLUDED

#include <sys/types.h>

typedef struct pspbrowse_item
{
    size_t name_size;
    char *name;
    size_t data_size;
    char *data;    
} pspbrowse_item_t;

typedef struct pspbrowse pspbrowse_t;

pspbrowse_t *pspbrowse_open(int fd);
int pspbrowse_close(pspbrowse_t *pspb);
void pspbrowse_reset(pspbrowse_t *pspb);
const pspbrowse_item_t *pspbrowse_next_item(pspbrowse_t *pspb);
const char *pspbrowse_title(pspbrowse_t *pspb);

#endif /* ndef PSPBROWSE_INCLUDED */
