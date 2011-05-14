/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <libsoup/soup.h>

#ifndef _HAVE_RAINMAKER_CLIENT_H

#include "rainmaker-scenario.h"
#include "rainmaker-scoreboard.h"

typedef struct _rmClient {
    SoupSession  *session;
    rmScoreboard *scoreboard;
} rmClient;

rmClient*     rm_client_new();
void          rm_client_set_logger(rmClient *client, SoupLogger *logger);
void          rm_client_free(rmClient *client);
void          rm_client_run_scenario(rmClient *client, rmScenario *scenario);

#define _HAVE_RAINMAKER_CLIENT_H
#endif

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
