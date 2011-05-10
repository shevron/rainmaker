#include <glib.h>
#include <libsoup/soup.h>

#ifndef _HAVE_RAINMAKER_REQUEST_H

/* {{{ typedef struct rmRequest 
 * 
 * Rainmaker request struct
 */
typedef struct _rmRequest {
    const gchar        *method;
    SoupURI            *url;
    SoupMessageHeaders *headers;
    gchar              *body;
    gsize               bodyLength;
} rmRequest;
/* }}} */

/* {{{ Error quark and codes */
#define RM_ERROR_REQ g_quark_from_static_string("rainmaker-request-error")

enum { 
    RM_ERROR_REQ_INVALID_URI
};
/* }}} */

rmRequest*  rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error);
void        rm_request_free(rmRequest *req);

#define _HAVE_RAINMAKER_REQUEST_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
