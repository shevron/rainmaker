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
    GError       *err = NULL;
    gint          i;

    g_type_init();

    sc = rm_scenario_new();
    sc->baseUrl = soup_uri_new("http://localhost/");

    req = rm_request_new("GET", "/", sc->baseUrl, &err);
    if (! req) goto exitwitherror;
    rm_scenario_add_request(sc, req);

    req = rm_request_new("GET", "/devcloud/", sc->baseUrl, &err);
    if (! req) goto exitwitherror;
    rm_scenario_add_request(sc, req);

    req = rm_request_new("GET", "/devcloud/container/create", sc->baseUrl, &err);
    if (! req) goto exitwitherror;
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

    rm_scenario_free(sc);
    rm_scoreboard_free(score);

    return 0;

exitwitherror:
    rm_scenario_free(sc);
    if (err != NULL) { 
        fprintf(stderr, "ERROR: %s\n", err->message);
    }

    return 1;
}

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
