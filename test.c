#include "pspbrowse.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int fd;
    pspbrowse_t *pspb;
    const pspbrowse_item_t *item;
    
    fd = open("pspbrwse.jbf", O_RDONLY);
    pspb = pspbrowse_open(fd);
    
    while((item = pspbrowse_next_item(pspb)))
    {
        printf("[%s]\n", item->name);
    }
    
    fd = pspbrowse_close(pspb);
    close(fd);
    
    return 0;
}
