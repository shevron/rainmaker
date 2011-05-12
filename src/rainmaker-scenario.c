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
#include "rainmaker-scenario.h"
#include "rainmaker-client.h"

/* {{{ static void rm_scenario_free_request(gpointer reqptr, gpointer user_data);
 *
 * Wrapper around rm_request_free that can be used by g_slist_foreach()
 */
static void rm_scenario_free_requests(gpointer reqptr, gpointer user_data)
{
    return rm_request_free((rmRequest *) reqptr);
}
/* rm_scenario_free_request }}} */

/* {{{ rmScenario* rm_scenario_new()
 *
 * Allocate memory for a new scenario object
 */
rmScenario* rm_scenario_new()
{
    rmScenario *scn;

    scn = g_malloc(sizeof(rmScenario));
    scn->requests = NULL;
    scn->logger   = NULL;

    scn->persistCookies  = FALSE;
    scn->failOnHttpError = TRUE;
    scn->failOnTcpError  = TRUE;

    return scn;
}
/* rm_scenario_new }}} */

/* {{{ void rm_scenario_free(rmScenario *scenario)
 *
 * Clear a scenario struct and free all allocated memory. Will also
 * free up any memory used by request structs
 */
void rm_scenario_free(rmScenario *scenario)
{
    // Free all requests
    g_slist_foreach(scenario->requests, (GFunc) rm_scenario_free_requests, NULL);
    g_slist_free(scenario->requests);

    g_free(scenario);
}
/* rm_scenario_free }}} */

/* {{{ void rm_scenario_add_request(rmScenario *scenario, rmRequest *request)
 *
 * Append a request to a scenario's list of requests
 */
void rm_scenario_add_request(rmScenario *scenario, rmRequest *request)
{
    g_assert(scenario != NULL);
    g_assert(request != NULL);

    scenario->requests = g_slist_append(scenario->requests, (gpointer) request);
}
/* rm_scenario_add_request }}} */

/* {{{ rmScoreboard* rm_scenario_run(rmScenario *scenario)
 *
 * Run scenario, returning the results in a scoreboard object
 */
rmScoreboard* rm_scenario_run(rmScenario *scenario)
{
    rmClient      *client;
    GSList        *rlNode;
    rmScoreboard  *sb;
    guint          status, i;
    SoupCookieJar *cookieJar = NULL;

    client = rm_client_new();

    if (scenario->logger) {
        rm_client_set_logger(client, scenario->logger);
    }

    // Enable cookie persistence if needed
    if (scenario->persistCookies) {
        cookieJar = soup_cookie_jar_new();
        soup_session_add_feature(client->session, (SoupSessionFeature *) cookieJar);
    }

    for (rlNode = scenario->requests; rlNode; rlNode = rlNode->next) {
        g_assert(rlNode->data != NULL);
        g_assert(((rmRequest *) rlNode->data)->repeat >= 1);

        for (i = 0; i < ((rmRequest *) rlNode->data)->repeat; i++) {
            status = rm_client_send_request(client, (rmRequest *) rlNode->data);

            if ((status < 100 && scenario->failOnTcpError) ||
                (status >= 400 && scenario->failOnHttpError)) {

                client->scoreboard->failed = TRUE;
                break;
            }
        }
    }

    // Copy results into new scoreboard, before freeing client
    sb = rm_scoreboard_new();
    rm_scoreboard_merge(sb, client->scoreboard);

    if (cookieJar) g_object_unref(cookieJar);
    rm_client_free(client);

    return sb;
}
/* rm_scenario_run }}} */

/**
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */

