#include <glib.h>
#include <libsoup/soup.h>

#ifndef _HAVE_RAINMAKER_REQUEST_H

/* {{{ typedef struct rmRequest 
 * 
 * Rainmaker request struct
 */
typedef struct _rmRequest {
    gchar              *method;
    SoupURI            *url;
    SoupMessageHeaders *headers;
    gchar              *body;
    gsize               bodyLength;
} rmRequest;
/* }}} */

rmRequest*  rm_request_new(gchar *url, SoupURI *baseUrl);
void        rm_request_free(rmRequest *req);

#define _HAVE_RAINMAKER_REQUEST_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
