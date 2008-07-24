#include "pspbrowse.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

struct pspbrowse
{
    int fd;
    char *title;
    pspbrowse_item_t item;
};

pspbrowse_t *pspbrowse_open(int fd)
{
    pspbrowse_t *pspb;
    char header[0x0400];
    static const char id[16] = "JASC BROWS FILE";
    
    lseek(fd, 0, SEEK_SET);
    if(read(fd, header, sizeof(header)) < sizeof(header))
        return NULL;
        
    if(memcmp(header, id, 0x0010) != 0)
        return NULL;
        
    pspb = (pspbrowse_t*)malloc(sizeof(pspbrowse_t));
    pspb->fd = fd;
    memset(&pspb->item, 0, sizeof(pspb->item));
    header[sizeof(header) - 1] = '\0';
    pspb->title = strdup(header + 0x0017);
        
    return pspb;
}

int pspbrowse_close(pspbrowse_t *pspb)
{
    int fd;
    
    if(!pspb)
        return -1;

    fd = pspb->fd;
    free(pspb->title);
    free(pspb->item.name);
    free(pspb->item.data);
    free(pspb);
    
    return fd;
}

void pspbrowse_reset(pspbrowse_t *pspb)
{
    lseek(pspb->fd, 0x0400, SEEK_SET);
}

const pspbrowse_item_t *pspbrowse_next_item(pspbrowse_t *pspb)
{
    /* Read item name */
    if(read(pspb->fd, &pspb->item.name_size, 4) < 4 || pspb->item.name_size < 0 )
        return NULL;
    free(pspb->item.name);
    if((pspb->item.name = (char*)malloc(pspb->item.name_size + 1)) == NULL)
        return NULL;    
    if(read(pspb->fd, pspb->item.name, pspb->item.name_size) < pspb->item.name_size)
        return NULL;
    pspb->item.name[pspb->item.name_size] = '\0';

    /* Skip item metadata */
    lseek(pspb->fd, 44, SEEK_CUR);
    
    /* Read item data */
    if(read(pspb->fd, &pspb->item.data_size, 4) < 4 || pspb->item.data_size < 0 )
        return NULL;
    free(pspb->item.data);
    if((pspb->item.data = (char*)malloc(pspb->item.data_size + 1)) == NULL)
        return NULL;    
    if(read(pspb->fd, pspb->item.data, pspb->item.data_size) < pspb->item.data_size)
        return NULL;
        
    return &pspb->item;
}


const char *pspbrowse_title(pspbrowse_t *pspb)
{
    return pspb->title;
}
