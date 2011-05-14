/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-request.h"
#include "rainmaker-scenario.h"
#include "rainmaker-client.h"

/// Wrapper around rm_request_free that can be used by g_slist_foreach(). This
/// is called from rm_screnario_free() when freeing requests.
static void rm_scenario_free_requests(gpointer reqptr, gpointer user_data)
{
    return rm_request_free((rmRequest *) reqptr);
}

/// Allocate memory for a new scenario object
rmScenario* rm_scenario_new()
{
    rmScenario *scn;

    scn = g_malloc(sizeof(rmScenario));
    scn->requests           = NULL;
    scn->persistCookies     = FALSE;
    scn->failOnHttpError    = TRUE;
    scn->failOnHttpRedirect = FALSE;
    scn->failOnTcpError     = TRUE;

    return scn;
}

/// Clear a scenario struct and free all allocated memory. Will also free up
/// any memory used by attached requests.
void rm_scenario_free(rmScenario *scenario)
{
    // Free all requests
    g_slist_foreach(scenario->requests, (GFunc) rm_scenario_free_requests, NULL);
    g_slist_free(scenario->requests);

    g_free(scenario);
}

/// Append a request to a scenario's list of requests
void rm_scenario_add_request(rmScenario *scenario, rmRequest *request)
{
    g_assert(scenario != NULL);
    g_assert(request != NULL);

    scenario->requests = g_slist_append(scenario->requests, (gpointer) request);
}

// vim:ts=4:expandtab:cindent:sw=2
