/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"
#include "rainmaker-request.h"
#include "rainmaker-client.h"

// Options set through command line args
typedef struct _cmdlineArgs {
    guint     clients;
    guint     repeat;
    gboolean  keepcookies; 
    guint     verbosity;
    gchar    *scenarioFile; 
} cmdlineArgs; 

// Text for different HTTP response code classes
static gchar* respcodes[] = {
    "TCP ERROR",
    "Informational",
    "Success",
    "Redirection",
    "Client Error",
    "Server Error"
};

static gboolean parse_args(int argc, char *argv[], cmdlineArgs *options)
{
    GOptionContext *ctx;
    GError         *error = NULL;
    gboolean        res;

    bzero(options, sizeof(cmdlineArgs));

    GOptionEntry    arguments[] = {
        {"clients", 'c', 0, G_OPTION_ARG_INT, &options->clients,
            "number of concurrent clients to run", NULL},
        {"repeat", 'r', 0, G_OPTION_ARG_INT, &options->repeat,
            "number of times to repeat entire scenario", NULL},
        {"keep-cookies", 'C', 0, G_OPTION_ARG_NONE, &options->keepcookies,
            "keep cookies between scenario repeats (per client)", NULL},
        {"verbose", 'v', 0, G_OPTION_ARG_INT, &options->verbosity, 
            "produce verbose output", "level (0-4)"},
        { NULL }
    };

    ctx = g_option_context_new("<scenario file>");
    g_option_context_set_summary(ctx, 
        PACKAGE_NAME " HTTP load testing tool, version " PACKAGE_VERSION);
    g_option_context_set_help_enabled(ctx, TRUE);
    g_option_context_add_main_entries(ctx, arguments, NULL);

    // Parse options
    res = g_option_context_parse(ctx, &argc, &argv, &error);
    g_option_context_free(ctx);

    if (! res) {
        g_printerr("failed parsing arguments: %s\n", error->message);
        return FALSE;
    }

    // Get remaining argument
    if (argc != 2) {
        g_printerr("error: no scenario file specified, run with --help for help\n");
        return FALSE;
    }
    options->scenarioFile = argv[1];
 
    // Check verbosity value
    if (options->verbosity < 0 || options->verbosity > 4) { 
        g_printerr("error: verbosity must be between 0 and 4\n");
        return FALSE;
    }

   
    return TRUE;
}


int main(int argc, char *argv[])
{
    cmdlineArgs   options;
    rmScenario   *sc;
    rmRequest    *req;
    rmScoreboard *score;
    GError       *err = NULL;
    gint          i;
    gboolean      failed = FALSE;

    g_type_init();

    if (! parse_args(argc, argv, &options)) { 
        return 1;
    }

    sc = rm_scenario_xml_read_file(options.scenarioFile, &err);
    if (! sc) goto exitwitherror;

    printf("Running scenario... ");
    score = rm_scenario_run(sc);
    if (score->failed) { 
        printf("TEST FAILED!\n");
    } else {
        printf("Done.\n");
    }

    // Print out scoreboard
    printf("Totral requests: %u\n", score->requests);
    printf("Elapsed Time:    %lf\n", score->elapsed); 
    printf("Response Codes:\n");
    for (i = 0; i < 6; i++) {
        if (score->resp_codes[i] > 0)
            printf("  %uxx %-15s: %u\n", i, respcodes[i], score->resp_codes[i]);
    }

    failed = score->failed;

    rm_scenario_free(sc);
    rm_scoreboard_free(score);

    return (failed ? 100 : 0);

exitwitherror:
    if (err != NULL) { 
        fprintf(stderr, "ERROR: %s\n", err->message);
    }

    return 2;
}

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
