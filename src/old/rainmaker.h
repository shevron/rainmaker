/**
 * rainmaker HTTP load generator
 * Copyright (c) 2010 Shahar Evron
 *
 * rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <libsoup/soup.h>

#include "config.h"

#ifndef _HAVE_RAINMAKER_H

#define RAINMAKER_USERAGENT "rainmaker/" PACKAGE_VERSION " "

typedef struct _rmHeader {
    gchar            *name;
    gchar            *value;
    struct _rmHeader *next;
} rmHeader;

typedef struct _rmRequest {
    const gchar *method;
    SoupURI     *url;
    rmHeader    *headers;
    gchar       *useragent;
    gchar       *ctype;
    gchar       *body;
    gsize        bodysize;
    gboolean     freebody;
    gboolean     savecookies;
} rmRequest;

typedef struct _rmResults {
    guint    respcodes[5];
    guint    clients;
    guint    total_reqs;
    gdouble  total_time;
} rmResults;

typedef struct _rmClient {
    rmRequest *request;
    rmResults *results;
    guint      repeats;
    gboolean   stoponerror;
    gboolean   done;
    GError    *error;
} rmClient;

/* Function Declerations */
rmRequest *rm_request_new();
void       rm_request_free(rmRequest *request);
rmHeader  *rm_header_new(gchar *name, gchar *value);
void       rm_header_free_all(rmHeader *header);
rmResults *rm_results_new();
rmResults *rm_results_add(rmResults *total, rmResults *add);
void       rm_results_free(rmResults *results);
rmClient  *rm_client_init(guint repeats, rmRequest *request);
void       rm_client_run(rmClient *client);
void       rm_client_free(rmClient *client);

#define _HAVE_RAINMAKER_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */

