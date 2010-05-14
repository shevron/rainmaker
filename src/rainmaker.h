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

typedef struct _rmGlobals {
    guint        requests;
	guint        clients;
    guint        tcount;
    SoupURI     *url;
    const gchar *method;
    gchar       *body;
    gsize        bodysize;
	gboolean     freebody;
    gchar       *ctype;
    GMutex      *tcmutex;
} rmGlobals;

rmGlobals *globals;

void rm_client_run(rmClient *client);

#define _HAVE_RAINMAKER_H
#endif
