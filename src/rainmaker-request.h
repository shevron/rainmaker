/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <glib.h>
#include <libsoup/soup.h>

#ifndef RAINMAKER_REQUEST_H_

typedef struct _rmHeader {
    gchar    *name;
    gchar    *value;
    gboolean  replace;
} rmHeader;

typedef enum {
    RM_REQUEST_PARAM_INT,
    RM_REQUEST_PARAM_FLOAT,
    RM_REQUEST_PARAM_BOOLEAN,
    RM_REQUEST_PARAM_NULL,
    RM_REQUEST_PARAM_STRING,
    RM_REQUEST_PARAM_ARRAY,
    RM_REQUEST_PARAM_OBJECT, // Can be handled by JSON and url encoding
    RM_REQUEST_PARAM_FILE    // Can only be handled by multipart encoding
} rmRequestParamType;

typedef struct _rmRequestParam {
    rmRequestParamType  type;
    gchar              *name;
    union {
        gchar          *strValue;
        gint            intValue;
        gdouble         floatValue;
        gboolean        boolValue;
        GSList         *arrayValue;
        GSList         *objectValue;
        gchar          *filePath;
    };
} rmRequestParam;

/// Rainmaker request struct
typedef struct _rmRequest {
    GQuark    method;      ///< request method
    SoupURI  *url;         ///< request URL
    GSList   *headers;     ///< request headers
    GQuark    bodyType;    ///< request body content-type
    gchar    *body;        ///< request body
    gsize     bodyLength;  ///< request body size in bytes
    gboolean  freeBody;    ///< do we need to free the body when done?
    guint     repeat;      ///< how many times to repeat the request
} rmRequest;

/// Error Quark for request related errors
#define RM_ERROR_REQ g_quark_from_static_string("rainmaker-request-error")

/// Request related error codes
enum {
    RM_ERROR_REQ_INVALID_URI,
    RM_ERROR_REQ_INVALID_ENCTYPE
};

rmHeader*       rm_header_new(const gchar *name, const gchar *value);
void            rm_header_copy_to_request(rmHeader *header, rmRequest *dest);
void            rm_header_free(rmHeader *header);
rmRequestParam* rm_request_param_new(rmRequestParamType type);
void            rm_request_param_free(rmRequestParam *param);
gchar*          rm_request_encode_params(const GSList *params, GQuark encoding, gsize *bodyLength, GError **error);
rmRequest*      rm_request_new(const gchar *method, gchar *url, const SoupURI *baseUrl, GError **error);
void            rm_request_add_header(rmRequest *request, const gchar *name, const gchar *value, gboolean reaplce);
void            rm_request_free(rmRequest *req);

// This can go away if we decide to bump the glib version requirement to 2.28
void            rm_gslist_free_full(GSList *list, GDestroyNotify free_func);

#define RAINMAKER_REQUEST_H_
#endif

// vim:ts=4:expandtab:cindent:sw=2
