/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <stdio.h>

#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"
#include "rainmaker-request.h"
#include "rainmaker-client.h"

static gchar* respcodes[] = {
    "TCP ERROR",
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

    if (argc != 2) { 
        fprintf(stderr, "Usage: %s <scenario file>\n", argv[0]);
        return 1;
    }

    g_type_init();

    sc = rm_scenario_xml_read_file(argv[1], &err);
    if (! sc) goto exitwitherror;

    printf("Running scenario...\n");
    score = rm_scenario_run(sc);
    printf("Done!\n");

    // Print out scoreboard
    printf("Totral requests: %u\n", score->requests);
    printf("Elapsed Time:    %lf\n", score->elapsed); 
    printf("Response Codes:\n");
    for (i = 0; i < 6; i++) {
        if (score->resp_codes[i] > 0)
            printf("  %uxx %-15s: %u\n", i, respcodes[i], score->resp_codes[i]);
    }

    rm_scenario_free(sc);
    rm_scoreboard_free(score);

    return 0;

exitwitherror:
    if (err != NULL) { 
        fprintf(stderr, "ERROR: %s\n", err->message);
    }

    return 2;
}

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
