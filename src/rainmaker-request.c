/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-request.h"

rmHeader* rm_header_new(const gchar *name, const gchar *value)
{
    rmHeader *header;

    header = g_malloc(sizeof(rmHeader));
    header->name  = g_strdup(name);
    header->value = g_strdup(value);
    header->replace = FALSE;

    return header;
}

void rm_header_free(rmHeader *header)
{
    g_free(header->name);
    g_free(header->value);
    g_free(header);
}

void rm_request_add_header(rmRequest *request, const gchar *name, const gchar *value, gboolean replace)
{
    rmHeader *header;

    header = rm_header_new(name, value);
    header->replace = replace;
    request->headers = g_slist_append(request->headers, (gpointer) header);
}

/* {{{ rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error)
 *
 * Create a new request struct with a defined URL
 */
rmRequest* rm_request_new(gchar *method, gchar *url, SoupURI *baseUrl, GError **error)
{
    rmRequest *req;

    req = g_malloc(sizeof(rmRequest));

    req->headers    = NULL;
    req->method     = method;
    req->body       = NULL;
    req->bodyLength = 0;
    req->freeBody   = FALSE;
    req->freeMethod = FALSE;

    if (baseUrl == NULL) {
        req->url = soup_uri_new(url);
    } else {
        req->url = soup_uri_new_with_base(baseUrl, url);
    }

    if (! SOUP_URI_VALID_FOR_HTTP(req->url)) {
        g_set_error(error, RM_ERROR_REQ, RM_ERROR_REQ_INVALID_URI,
            "invalid URI provided for request: '%s'. Are you missing a base URL?", url);
        rm_request_free(req);
        return NULL;
    }

    return req;
}
/* rm_request_new }}} */

void rm_header_copy_to_request(rmHeader *header, rmRequest *dest)
{
    rm_request_add_header(dest, header->name, header->value, header->replace);
}

/* {{{ void rm_request_free(rmRequest *req)
 *
 * Free a request struct and all related memory
 */
void rm_request_free(rmRequest *req)
{
    if (req->url != NULL) soup_uri_free(req->url);
    g_slist_free_full(req->headers, (GDestroyNotify) rm_header_free);

    if (req->freeBody && req->body != NULL)
        g_free(req->body);

    if (req->freeMethod && req->method != NULL)
        g_free(req->method);

    g_free(req);
}
/* rm_request_free }}} */

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
