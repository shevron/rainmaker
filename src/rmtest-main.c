/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"
#include "rainmaker-request.h"
#include "rainmaker-client.h"
#include "rainmaker-scoreboard.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef RM_MAX_CLIENTS
#define RM_MAX_CLIENTS 100
#endif

/// Options set through command line arguments
typedef struct _cmdlineArgs {
    guint     clients;
    guint     repeat;
    gboolean  keepcookies;
    guint     verbosity;
    gchar    *scenarioFile;
} cmdlineArgs;

/// This struct is used to pass data to new client threads
typedef struct _rmThreadClient {
    rmClient   *client;
    rmScenario *scenario;
} rmThreadClient;

/// Verbosity levels
enum {
    VERBOSITY_SILENT,
    VERBOSITY_SUMMARY,
    VERBOSITY_MINIMAL,
    VERBOSITY_HEADERS,
    VERBOSITY_FULL
};

/// Parse command line arguments
static gboolean parse_args(int argc, char *argv[], cmdlineArgs *options)
{
    GOptionContext *ctx;
    GError         *error = NULL;
    gboolean        res;

    // Set some defaults
    bzero(options, sizeof(cmdlineArgs));
    options->clients = 1;

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
        g_printerr("ERROR: failed parsing arguments: %s\n", error->message);
        return FALSE;
    }

    // Get remaining argument
    if (argc != 2) {
        g_printerr("ERROR: no scenario file specified, run with --help for help\n");
        return FALSE;
    }
    options->scenarioFile = argv[1];

    // Check verbosity value
    if (options->verbosity < 0 || options->verbosity > 4) {
        g_printerr("ERROR: verbosity must be between 0 and 4\n");
        return FALSE;
    }

    if (options->clients < 1|| options->clients > RM_MAX_CLIENTS) {
        g_printerr("ERROR: number of clients must be between 1 and %u\n", RM_MAX_CLIENTS);
        return FALSE;
    }

    return TRUE;
}

static void log_printer(SoupLogger *logger, SoupLoggerLogLevel level,
    char direction, const char *data, gpointer user_data)
{
    // Logging should be done to STDERR
    g_printerr("%c %s\n", direction, data);
}

static SoupLogger* create_logger(cmdlineArgs *args)
{
    SoupLogger *logger;

    static SoupLoggerLogLevel logLevels[] = {
    	SOUP_LOGGER_LOG_MINIMAL,
	    SOUP_LOGGER_LOG_HEADERS,
	    SOUP_LOGGER_LOG_BODY
    };

    logger = soup_logger_new(logLevels[args->verbosity - VERBOSITY_MINIMAL], -1);
    soup_logger_set_printer(logger, log_printer, NULL, NULL);

    return logger;
}

static void run_client(rmThreadClient *client)
{
    rm_client_run_scenario(client->client, client->scenario);
}

int main(int argc, char *argv[])
{
    cmdlineArgs      options;
    rmScenario      *sc;
    rmThreadClient **clients;
    GThread        **threads;
    rmScoreboard    *total;
    GError          *err = NULL;
    SoupLogger      *logger = NULL;
    gint             i;
    gboolean         failed = FALSE;

    // Text for different HTTP response code classes
    static gchar* respcodes[] = {
        "TCP ERROR",
        "Informational",
        "Success",
        "Redirection",
        "Client Error",
        "Server Error"
    };

    g_type_init();
    g_thread_init(NULL);

    if (! parse_args(argc, argv, &options)) {
        return 1;
    }

    sc = rm_scenario_xml_read_file(options.scenarioFile, &err);
    if (! sc) goto exitwitherror;

    // Set up logger
    if (options.verbosity > VERBOSITY_SUMMARY) {
        logger = create_logger(&options);
    }

    printf("Running scenario... ");

    // Create and run all clients
    clients = g_malloc(sizeof(rmThreadClient *) * options.clients);
    threads = g_malloc(sizeof(GThread *) * options.clients);
    for (i = 0; i < options.clients; i++) {
        clients[i] = g_malloc(sizeof(rmThreadClient));
        clients[i]->client   = rm_client_new();
        clients[i]->scenario = sc;
        if (logger) {
            // Attach logger
            soup_session_add_feature(clients[i]->client->session, (SoupSessionFeature *) logger);
        }

        threads[i] = g_thread_create((GThreadFunc) run_client, (gpointer) clients[i], TRUE, NULL);
    }

    total = rm_scoreboard_new();

    // Wait for all threads to finish
    for (i = 0; i < options.clients; i++) {
        g_thread_join(threads[i]);
    }

    // Free all clients
    for (i = 0; i < options.clients; i++) {
        rm_scoreboard_merge(total, clients[i]->client->scoreboard);
        rm_client_free(clients[i]->client);
        g_free(clients[i]);
    }

    g_free(clients);
    g_free(threads);

    if (total->failed) {
        printf("TEST FAILED!\n");
    } else {
        printf("done.\n");
    }

    // Print out scoreboard
    printf("Total requests: %u\n", total->requests);
    printf("Elapsed Time:   %lf\n", total->elapsed);
    printf("Response Codes:\n");
    for (i = 0; i < 6; i++) {
        if (total->resp_codes[i] > 0)
            printf("  %uxx %-15s: %u\n", i, respcodes[i], total->resp_codes[i]);
    }

    failed = total->failed;

    if (logger) g_object_unref(logger);
    rm_scenario_free(sc);
    rm_scoreboard_free(total);

    return (failed ? 100 : 0);

exitwitherror:
    if (err != NULL) {
        fprintf(stderr, "ERROR: %s\n", err->message);
    }

    return 2;
}

// vim:ts=4:expandtab:cindent:sw=2
