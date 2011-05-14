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
#include "rainmaker-client.h"

#ifndef _HAVE_RAINMAKER_SCENARIO_H

/* {{{ typedef struct rmScenario
 *
 * Scenario struct
 */
typedef struct _rmScenario {
    GSList     *requests;
    SoupLogger *logger;
    gboolean    persistCookies;
    gboolean    failOnHttpError;
    gboolean    failOnTcpError;
} rmScenario;
/* }}} */

rmScenario*   rm_scenario_new();
rmScoreboard* rm_scenario_run(rmScenario *scenario);
rmScoreBoard* rm_scenario_run_multi(rmScenario *scenario, guint clients);
void          rm_scenario_add_request(rmScenario *scenario, rmRequest *request);
void          rm_scenario_free(rmScenario *scenario);

#define _HAVE_RAINMAKER_SCENARIO_H
#endif;

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
