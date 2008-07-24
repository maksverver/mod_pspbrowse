#include "pspbrowse.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <apr_strings.h>
#include <httpd.h>
#include <http_config.h>

static const char hexdigits[] = "0123456789ABCDEF";

static char *htmlspecialchars(const char *src)
{
    char *result, *dst;
    int src_size, dst_size;
    
    for(src_size = dst_size = 0; src[src_size]; ++src_size)
        switch(src[src_size])
        {
        case '&': dst_size += 5; break;
        case '"': dst_size += 6; break;
        case '<': dst_size += 4; break;
        case '>': dst_size += 4; break;
        default:  dst_size += 1; break;
        }
        
    result = dst = malloc(dst_size + 1);
    for( ; *src; ++src)
        switch(*src)
        {
        case '&': strcpy(dst, "&amp;");   dst += 5; break;
        case '"': strcpy(dst, "&quote;"); dst += 6; break;
        case '<': strcpy(dst, "&lt;");    dst += 4; break;
        case '>': strcpy(dst, "&gt;");    dst += 4; break;
        default:  *dst++ = *src; break;
        }
    *dst = '\0';
        
    return result;
}

static int isurlsafe(int chr)
{
    return isalnum(chr) || chr == '.' || chr == '-' || chr == '_';
}

static char *rawurlencode(const char *src)
{
    char *result, *dst;
    int src_size, dst_size;
    
    for(src_size = dst_size = 0; src[src_size]; ++src_size)
        dst_size += isurlsafe(src[src_size]) ? 1 : 3;
        
    result = dst = malloc(dst_size + 1);
    for( ; *src; ++src)
        if(isurlsafe(*src))
        {
            *dst++ = *src;
        }
        else
        {
            *dst++ = '%';
            *dst++ = hexdigits[ (((unsigned char)*src) >> 4) & 15 ];
            *dst++ = hexdigits[  ((unsigned char)*src)       & 15 ];
        }
    *dst = '\0';
        
    return result;
}

static char *rawurldecode(const char *src)
{
    char *result, *dst;
    
    result = dst = malloc(strlen(src) + 1);
    
    while(*src)
    {
        char *h, *l;
        if( src[0] == '%' && (h = strchr(hexdigits, src[1])) &&
            (l = strchr(hexdigits, src[2])) )
        {
            *dst++ = ((unsigned char)(h - hexdigits)) << 4 |
                     ((unsigned char)(l - hexdigits));
            src += 3;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
        
    return result;
}

static int generate_index(pspbrowse_t *pspb, request_rec *r)
{
    char *title;
    const pspbrowse_item_t *item;
    DIR *dir;

    title = htmlspecialchars(pspbrowse_title(pspb));

    ap_set_content_type(r, "text/html; charset=utf-8");
    ap_rprintf( r,
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\r\n"
        "<html><head><title>%s</title>"
        "<style title=\"Default\" type=\"text/css\">"
        "body { text-align: center; font: 10.5pt sans-serif; } "
        "img { border: 0; } "
        "a { text-decoration: none; color: #4040FF; } "
        "a:hover { text-decoration: underline; } "
        "div.thumbnail { display: inline; float: left; border: 2px solid #E0E0FF; "
        "background: #F0F0FF; margin: 0.5em; padding: 0.5em; } "        
        "span.title { display: block; font-weight: bold; font-variant: small-caps; } "
        "div.directory { font-weight: bold; font-variant: small-caps; text-align: left; margin: 0.5em; } "
        "</style></head><body><h1>%s</h1>", title, title );

    if((dir = opendir(dirname(r->filename))))
    {
        struct dirent entry, *result;
        char *name, *url;
        
        while( readdir_r(dir, &entry, &result), result )
            if(entry.d_type & DT_DIR && strcmp(entry.d_name, ".") != 0)
            {
                name = htmlspecialchars(entry.d_name);
                url  = rawurlencode(entry.d_name);
                
                ap_rprintf( r,
                    "<div class=\"directory\"><a href=\"%s\">%s</a></div>\n",
                    url, name );
                
                free(name);
                free(url);
            }            
        closedir(dir);
    }

    while((item = pspbrowse_next_item(pspb)) != NULL)
    {
        char *title, *url;
        
        title = htmlspecialchars(item->name);
        url   = rawurlencode(item->name);
                        
        ap_rprintf( r,
            "<div class=\"thumbnail\"><a href=\"%s\">"
            "<img src=\"?%s\" alt=\"Thumbnail\" title=\"%s\" /></a>"
            "<span class=\"title \">%s</span></div>\n",
            url, url, title, title );
            
        free(title);
        free(url);
    }
    ap_rprintf(r, "</body></html>");
    
    free(title);
    
    return OK;
}

static int generate_image(pspbrowse_t *pspb, request_rec *r, const char *name)
{
    const pspbrowse_item_t *item;

    while((item = pspbrowse_next_item(pspb)) != NULL)
        if(strcmp(name, item->name) == 0)
        {
            ap_set_content_type(r, "image/jpeg");
            ap_rwrite(item->data, item->data_size, r);
            
            return OK;
        }

    return 404;
}


static int handler(request_rec *r)
{
    pspbrowse_t *pspb;
    int fd, result;
    char *query_string;
    
    if(strcmp(r->handler, "application/x-pspbrowse") != 0)
        return DECLINED;
        
    result = DECLINED;

    if( (fd = open(r->filename, O_RDONLY)) == -1 ||
        (pspb = pspbrowse_open(fd)) == NULL )
    {
        result = DECLINED;
    }
    else
    {
        query_string = r->args ? rawurldecode(r->args) : NULL;

        /* TODO: add HTTP headers: generator, last modified */
        /* TODO: check for if-modified-since */
        
        result = query_string ? generate_image(pspb, r, query_string)
                              : generate_index(pspb, r);

        pspbrowse_close(pspb);
        
        free(query_string);
    }
    if(fd != -1)
        close(fd);

    return result;
}

static void register_hooks()
{
    ap_hook_handler(handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA pspbrowse_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,			            /* create per-directory config structure */
    NULL,        		        /* merge per-directory config structures */
    NULL,			            /* create per-server config structure */
    NULL,			            /* merge per-server config structures */
    NULL,			            /* command apr_table_t */
    register_hooks              /* register hooks */
};
