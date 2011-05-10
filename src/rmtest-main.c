#include <glib.h>
#include <stdio.h>

#include "rainmaker-scenario.h"
#include "rainmaker-request.h"
#include "rainmaker-client.h"

static gchar* respcodes[] = {
    "Informational",
    "Success",
    "Redirection",
    "Client Error",
    "Server Error"
};

int main(int argc, char *argv[])
{
    rmScenario   *sc;
    rmRequest    *req;
    rmScoreboard *score;
    gint          i;

    g_type_init();

    sc = rm_scenario_new();

    req = rm_request_new("GET", "http://localhost/", sc->baseUrl);
    rm_scenario_add_request(sc, req);

    req = rm_request_new("GET", "http://localhost/devcloud", sc->baseUrl);
    rm_scenario_add_request(sc, req);

    req = rm_request_new("GET", "http://localhost/devcloud/container/create", sc->baseUrl);
    rm_scenario_add_request(sc, req);

    printf("Running scenario...\n");
    score = rm_scenario_run(sc);
    printf("Done!\n");

    // Print out scoreboard
    printf("Totral requests: %u\n", score->requests);
    printf("Elapsed Time:    %lf\n", score->elapsed); 
    printf("Response Codes:\n");
    for (i = 0; i < 5; i++) {
        if (score->resp_codes[i] > 0)
            printf("  %uxx %-15s: %u\n", i + 1, respcodes[i], score->resp_codes[i]);
    }

    return 0;
}

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
