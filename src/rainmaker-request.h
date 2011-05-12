/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <libsoup/soup.h>

#ifndef _HAVE_RAINMAKER_REQUEST_H

typedef struct _rmHeader {
    gchar    *name;
    gchar    *value;
    gboolean  replace;
} rmHeader;

/* {{{ typedef struct rmRequest
 *
 * Rainmaker request struct
 */
typedef struct _rmRequest {
    GQuark    method;
    SoupURI  *url;
    GSList   *headers;
    gchar    *body;
    gsize     bodyLength;
    gboolean  freeBody;
} rmRequest;
/* }}} */

/* {{{ Error Quark and codes */
#define RM_ERROR_REQ g_quark_from_static_string("rainmaker-request-error")

enum {
    RM_ERROR_REQ_INVALID_URI
};
/* }}} */

rmHeader*  rm_header_new(const gchar *name, const gchar *value);
void       rm_header_copy_to_request(rmHeader *header, rmRequest *dest);
void       rm_header_free(rmHeader *header);
rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error);
void       rm_request_add_header(rmRequest *request, const gchar *name, const gchar *value, gboolean reaplce);
void       rm_request_free(rmRequest *req);

#define _HAVE_RAINMAKER_REQUEST_H
#endif

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
