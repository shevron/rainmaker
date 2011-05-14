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

static gchar *rmRequestParamTypeNames[] = {
    "integer", "float",  "boolean", "null", "string",
    "array",   "object", "file"
};

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

rmRequestParam* rm_request_param_new(rmRequestParamType type)
{
    rmRequestParam *param;

    param = g_malloc0(sizeof(rmRequestParam));
    param->type = type;

    return param;
}

void rm_request_param_free(rmRequestParam *param)
{
    // TODO: this needs to be type aware once we have other types than string
    if (param->name != NULL) g_free(param->name);
    if (param->strValue != NULL) g_free(param->strValue);

    g_free(param);
}

static gchar* encode_params_urlencoded(const GSList *params, gsize *bodyLength, GError **error)
{
    GSList  *ptr;
    GString *body;
    gchar   *encstr;

    body = g_string_new("");

    for (ptr = (GSList *) params; ptr; ptr = ptr->next) {
        g_assert(ptr->data);
        rmRequestParam *param = (rmRequestParam *) ptr->data;

        if(param->type > RM_REQUEST_PARAM_STRING) {
            g_set_error(error, RM_ERROR_REQ, RM_ERROR_REQ_INVALID_ENCTYPE,
                "parameters of type '%s' cannot be encoded using %s",
                rmRequestParamTypeNames[param->type],
                "application/x-www-form-urlencoded");
            break;
        }

        // Add encoded parameter name
        encstr = soup_uri_encode(param->name, NULL);
        g_string_append_printf(body, "%s%s=", (body->len > 0 ? "&" : ""), encstr);
        g_free(encstr);

        // Add encoded parameter value, depending on type
        // TODO handle different types, for now all are string
        encstr = soup_uri_encode(param->strValue, NULL);
        g_string_append_printf(body, "%s", encstr);
        g_free(encstr);
    }

    if (*error != NULL) {
        return g_string_free(body, TRUE);
    }

    *bodyLength = body->len;

    return g_string_free(body, FALSE);
}

gchar* rm_request_encode_params(const GSList *params, GQuark encoding, gsize *bodyLength, GError **error)
{
    gchar *body;

    if (encoding == g_quark_from_static_string("application/x-www-form-urlencoded")) {
        body = encode_params_urlencoded(params, bodyLength, error);

    } else if (encoding == g_quark_from_static_string("multipart/form-data") ||
               encoding == g_quark_from_static_string("application/json")) {

        // TODO: implement handling of these encoding types
        g_set_error(error, RM_ERROR_REQ, RM_ERROR_REQ_INVALID_ENCTYPE,
            "unknown or not-yet-implemented form encoding method: '%s'",
            g_quark_to_string(encoding));
        return NULL;

    } else {
        g_set_error(error, RM_ERROR_REQ, RM_ERROR_REQ_INVALID_ENCTYPE,
            "unknown or not-yet-implemented form encoding method: '%s'",
            g_quark_to_string(encoding));
        return NULL;
    }

    return body;
}

/* {{{ rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error)
 *
 * Create a new request struct with a defined URL
 */
rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error)
{
    rmRequest *req;

    req = g_malloc(sizeof(rmRequest));

    req->method     = g_quark_from_string(method);
    req->headers    = NULL;
    req->bodyType   = 0;
    req->body       = NULL;
    req->bodyLength = 0;
    req->freeBody   = FALSE;
    req->repeat     = 1;

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

/// Copy a header to a request object's header list. This creates a full copy
/// of the header's name and value
///
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

    g_free(req);
}
/* rm_request_free }}} */

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
