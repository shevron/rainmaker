#include "config.h"

#ifndef _HAVE_RAINMAKER_H

typedef struct _rmClient {
    int     status_10x;
    int     status_20x;
    int     status_30x;
    int     status_40x;
    int     status_50x;
    double  timer;
} rmClient;

typedef struct _rmGlobals {
    int          requests;
    int          tcount;
    SoupURI     *url;
    const gchar *method;
    gchar       *body;
    gsize        bodysize;
    gchar       *ctype;
    GMutex      *tcmutex;
} rmGlobals;

rmGlobals *globals;

void rm_client_run(rmClient *client);

#define _HAVE_RAINMAKER_H
#endif
