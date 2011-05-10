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
    GSList   *requests;
    SoupURI  *baseUrl;
    gboolean  persistCookies;
    gboolean  failOnHttpError;
    gboolean  failOnTcpError;
} rmScenario;
/* }}} */

rmScenario*   rm_scenario_new();
rmScoreboard* rm_scenario_run(rmScenario *scenario);
void          rm_scenario_add_request(rmScenario *scenario, rmRequest *request);
void          rm_scenario_free(rmScenario *scenario);

#define _HAVE_RAINMAKER_SCENARIO_H
#endif;

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
