/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <glib.h>
#include <libsoup/soup.h>

#ifndef RAINMAKER_SCENARIO_H_

#include "rainmaker-request.h"

/// Scenario struct
typedef struct _rmScenario {
    GSList     *requests;
    gboolean    persistCookies;
    gboolean    failOnHttpError;
    gboolean    failOnHttpRedirect;
    gboolean    failOnTcpError;
} rmScenario;

rmScenario*   rm_scenario_new();
void          rm_scenario_add_request(rmScenario *scenario, rmRequest *request);
void          rm_scenario_free(rmScenario *scenario);

#define RAINMAKER_SCENARIO_H_
#endif;

// vim:ts=4:expandtab:cindent:sw=2
