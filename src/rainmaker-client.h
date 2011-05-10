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

#ifndef _HAVE_RAINMAKER_CLIENT_H

typedef struct _rmScoreboard { 
    guint   requests;
    guint   resp_codes[6];
    gdouble elapsed;
    GTimer *stopwatch;
} rmScoreboard; 

typedef struct _rmClient {
    SoupSession  *session;
    rmScoreboard *scoreboard;
} rmClient;

rmScoreboard *rm_scoreboard_new();
void          rm_scoreboard_merge(rmScoreboard *target, rmScoreboard *src);
void          rm_scoreboard_free(rmScoreboard *sb);
rmClient*     rm_client_new();
void          rm_client_free(rmClient *client);
guint         rm_client_send_request(rmClient *client, rmRequest *request);

#define _HAVE_RAINMAKER_CLIENT_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
