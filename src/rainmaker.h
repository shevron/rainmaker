/**
 * rainmaker HTTP load generator
 * Copyright (c) 2010 Shahar Evron
 *
 * rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include "config.h"

#ifndef _HAVE_RAINMAKER_H

typedef struct _rmClient {
    guint   status_10x;
    guint   status_20x;
    guint   status_30x;
    guint   status_40x;
    guint   status_50x;
    gdouble timer;
} rmClient;

typedef struct _rmHeader {
    gchar            *name;
    gchar            *value;
    struct _rmHeader *next;
} rmHeader;

typedef struct _rmGlobals {
    /* Execution Options */
    guint        requests;
    guint        clients;
    guint        tcount;
    GMutex      *tcmutex;
    
    /* Request Data */
    const gchar *method;
    SoupURI     *url;
    rmHeader    *headers;
    gchar       *ctype;
    gchar       *body;
    gsize        bodysize;
    gboolean     freebody;
} rmGlobals;

rmGlobals *globals;

void rm_client_run(rmClient *client);

#define _HAVE_RAINMAKER_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=syntax 
 */

