/**
 * rainmaker HTTP load generator
 * Copyright (c) 2010 Shahar Evron
 *
 * rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>

#include "config.h"
#include "rainmaker.h"

enum {
  STATUS_10x,
  STATUS_20x,
  STATUS_30x,
  STATUS_40x,
  STATUS_50x,
};

typedef struct _rmConfig {
    guint requests;
    guint clients;
} rmConfig;

typedef enum {
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
} rmMethod;

static char const *methods[] = {"GET", "POST", "PUT"};

enum {
    CTYPE_FORM_URLENCODED,
    CTYPE_APP_OCTET_STREAM,
};

static char const *ctypes[] = {
    "application/x-www-form-urlencoded",
    "application/octet-stream"
};

/* {{{ rm_header_new_from_string() - create a new header struct from string
 *
 * Will take in a "Header: Value" form string, parse / split it and create
 * a new rmHeader struct from it. 
 *
 * The input string is modified, and original
 * string is split into two strings by inserting a '\0' character to separate
 * header name and value. No additional memory is allocated to contain the 
 * name and value - so you must not attempt to free the input string before
 * releasing the rmHeader struct. 
 *
 * The returned rmHeader struct must be freed using rm_header_free_all().
 *
 * Will return NULL if the input string is not a valid header.
 */
static rmHeader* rm_header_new_from_string(gchar *string)
{
    gchar    *name, *value = NULL;
    rmHeader *header;
    int       i;

    name = string;

    for (i = 0; string[i] != '\0'; i++) {
        if (string[i] <= 0x20 || string[i] == 0x7f) {
          return NULL;
        }

        switch (string[i]) {
            case '(':
            case ')':
            case '<':
            case '>':
            case '@':
            case ',':
            case ';':
            case '\\':
            case '"':
            case '/':
            case '[':
            case ']':
            case '?':
            case '=':
            case '{':
            case '}':
                return NULL;
                break;

            case ':':
                string[i] = '\0';
                value = &string[i + 1];
                i--;
                break;
        }
    }

    g_strstrip(name);
    g_strstrip(value);

    if (strlen(name) == 0 || strlen(value) == 0) {
        return NULL;
    }

    header = rm_header_new(name, value);

    return header;
}
/* rm_header_new_from_string() }}} */

/* {{{ parse_load_cmd_args() - parse command line arguments and load config
 *
 */
static gboolean parse_load_cmd_args(int argc, char *argv[], rmConfig *config, rmRequest *request)
{
    GOptionContext  *context;
    GError          *error    = NULL;
    guint            clients  = 1, 
                     requests = 1;
    gchar           *postdata = NULL, 
                    *postfile = NULL,
                    *putfile  = NULL,
                    *ctype    = NULL;
    gchar          **headers  = NULL;
#ifdef HAVE_LIBSOUP_COOKIEJAR
    gboolean         savecookies = FALSE;
#endif

    /* Define and parse command line arguments */
    GOptionEntry    options[] = {
        {"clients",  'c', 0, G_OPTION_ARG_INT, &clients, 
            "number of concurrent clients to run", NULL},
        {"requests", 'r', 0, G_OPTION_ARG_INT, &requests, 
            "number of requests to send per client", NULL},
        {"header",   'H', 0, G_OPTION_ARG_STRING_ARRAY, &headers,
            "set a custom HTTP header, in 'Name: Value' format", NULL},
        {"postdata", 'P', 0, G_OPTION_ARG_STRING, &postdata, 
            "post data (will send a POST request)", NULL},
        {"postfile", 0,   0, G_OPTION_ARG_FILENAME, &postfile, 
            "read body from file and send as a POST request", NULL},
        {"putfile",  0,   0, G_OPTION_ARG_FILENAME, &putfile,
            "read body from file and send as a PUT request", NULL},
        {"ctype",    't', 0, G_OPTION_ARG_STRING, &ctype, 
            "content type to use in POST/PUT requests", NULL},
#ifdef HAVE_LIBSOUP_COOKIEJAR
        {"savecookies", 'C', 0, G_OPTION_ARG_NONE, &savecookies, 
            "enable per-client cookie persistence", NULL},
#endif
        { NULL }
    };

    context = g_option_context_new("<target URL>");
    g_option_context_set_summary(context, 
        PACKAGE_NAME " - a simple HTTP load generator, version " PACKAGE_VERSION);
    g_option_context_set_help_enabled(context, TRUE);
    g_option_context_add_main_entries(context, options, NULL);

    /* Parse ! */
    if (! g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("command line arguments parsing failed: %s\n", error->message);
        g_option_context_free(context);
        return FALSE;
    }

    g_option_context_free(context);

    if (argc < 2 || argc > 2) {
        g_printerr("error: no target URL specified, run with --help for help\n");
        return FALSE;
    }

    /* Set number of requests / clients */
    if (requests <= 1) {
        config->requests = 1; 
    } else {
        config->requests = requests;
    }

    if (clients <= 1) {
        config->clients = 1;
    } else {
        config->clients = clients;
    }

    /* Set method and body */
    if ((postdata != NULL && (postfile != NULL || putfile != NULL)) ||
        (postfile != NULL && putfile != NULL)) {
        
        g_printerr("error: you may only specify one of --postdata, --postfile and --putfile\n");
        return FALSE;
    }

    if (postdata != NULL) {
        g_assert(postfile == NULL);
        g_assert(putfile  == NULL);

        request->method   = methods[METHOD_POST];
        request->body     = postdata;
        request->bodysize = (gsize) strlen(postdata);
        request->ctype    = (gchar *) ctypes[CTYPE_FORM_URLENCODED];

    } else if (postfile != NULL) {
        g_assert(postdata == NULL);
        g_assert(putfile  == NULL);

        request->method   = methods[METHOD_POST];
        request->ctype    = (gchar *) ctypes[CTYPE_FORM_URLENCODED];
        if (! g_file_get_contents(postfile, &request->body, &request->bodysize, &error)) {
            g_printerr("Error loading body: %s\n", error->message);
            return FALSE;
        }
        request->freebody = TRUE;

    } else if (putfile != NULL) {
        g_assert(postdata == NULL);
        g_assert(postfile == NULL);

        request->method   = methods[METHOD_PUT];
        request->ctype    = (gchar *) ctypes[CTYPE_APP_OCTET_STREAM];
        if (! g_file_get_contents(putfile, &request->body, &request->bodysize, &error)) {
            g_printerr("error loading body: %s\n", error->message);
            return FALSE;
        }

        request->freebody = TRUE;

    } else {
        request->method = methods[METHOD_GET];
    }
    
    /* Set content type */
    if (request->body != NULL && ctype != NULL) {
        request->ctype = ctype;
    }

    /* Set user agent */
    /* TODO: make this a command line argument / config option */
    request->useragent = g_strdup(RAINMAKER_USERAGENT);
    
    /* Set headers */
    if (headers != NULL) {
        int i;
        gchar *orig_header;
        rmHeader *header, *prev = NULL;

        for (i = 0; headers[i] != NULL; i++) {
            orig_header = g_strdup(headers[i]);
            header = rm_header_new_from_string(headers[i]);
            if (header == NULL) {
                g_printerr("Error: inavlid header '%s'\n", orig_header);
                g_free(orig_header);
                return FALSE;
            }
            g_free(orig_header);

            if (request->headers == NULL) request->headers = header;
            if (prev != NULL) prev->next = header;
            prev = header;
        }
    }

    /* Set URL */
    if ((request->url = soup_uri_new(argv[1])) == NULL) {
        g_printerr("Error: unable to parse URL '%s'\n", argv[1]);
        return FALSE;
    }

#ifdef HAVE_LIBSOUP_COOKIEJAR
    request->savecookies = savecookies;
#endif

    return TRUE;
}
/* parse_load_cmd_args() }}} */

/* {{{ wait_for_clients() - wait for all clients to finish and return the results 
 *
 */
static rmResults* wait_for_clients(rmConfig *config, rmClient **clients, GError **error)
{
    rmResults *results;
    gint       i, total_reqs, done_reqs = 0;
    gboolean   done;
    gdouble    total_time;
    gdouble    current_load = 0;

    g_assert(*error == NULL);
    g_assert(clients != NULL);

    total_reqs = config->clients * config->requests;

    /* Check client status, loop until all clients are done */
    do {
        g_usleep(50000);
        done = TRUE;
        done_reqs = 0;
        total_time = 0;

        /* Iterate over all clients, check status */
        for (i = 0; clients[i] != NULL; i++) {
            if (clients[i]->error != NULL) {
                *error = clients[i]->error;
                break;
            }

            done        = (done && clients[i]->done);
            done_reqs  += clients[i]->results->total_reqs;
            total_time += clients[i]->results->total_time;
        }

        if (*error != NULL) break;
        
        if (total_time && config->clients) {
            current_load = done_reqs / (total_time / config->clients);
        }

        printf("[Sent %d/%d requests, running at %.3f req/sec]%10s", 
            done_reqs, total_reqs, current_load, "\r");
        fflush(stdout);

    } while (! done);

    if (*error != NULL) {
        return NULL;
    }
    
    /* sum up results */
    results = rm_results_new();
    for (i = 0; clients[i] != NULL; i++) {
        rm_results_add(results, clients[i]->results);
    }
    
    return results;
}
/* wait_for_clients() }}} */

/* {{{ main() - what do you think?
 *
 */
int main(int argc, char *argv[])
{
    rmClient **clients;
    rmConfig  *config;
    rmRequest *request;
    rmResults *results;
    GError    *error = NULL;
    gdouble     avg_load_client = 0;
    gdouble     avg_load_server = 0;
    gint        i;

    g_type_init();
    g_thread_init(NULL);

    request = rm_request_new();
    config = g_malloc(sizeof(rmConfig));

    /* Load configuration */
    if (! parse_load_cmd_args(argc, argv, config, request)) {
        exit(1);
    }

    /* Create client threads */
    clients = g_malloc(sizeof(rmClient*) * (config->clients + 1)); 
    for (i = 0; i < config->clients; i++) {
        clients[i] = rm_client_init(config->requests, request);
        g_thread_create((GThreadFunc) rm_client_run, clients[i], FALSE, NULL);
    }
    clients[config->clients] = NULL;

    /* Wait for all clients to finish */
    results = wait_for_clients(config, clients, &error);

    /* free clients, request and config */
    for (i = 0; clients[i] != NULL; i++) {
        rm_client_free(clients[i]);
    }
    g_free(clients);
    rm_request_free(request);
    g_free(config);

    if (results == NULL) {
        g_assert(error != NULL);
        g_printerr("Client error: %s\n", error->message);
        g_error_free(error);

        exit(2);
    }

    avg_load_client = 1 / (results->total_time / results->total_reqs);
    avg_load_server = 1 / ((results->total_time / results->clients) / results->total_reqs);

    /* Print out summary */
    if (results->total_reqs) {
        printf("\n------------------------------------------------------------------------\n");
        printf("Completed %d requests in %.3f seconds\n", results->total_reqs, (results->total_time / results->clients));
        printf("  Average load on server:  %.3f req/sec\n", avg_load_server);
        printf("  Average load per client: %.3f req/sec\n", avg_load_client);
        printf("------------------------------------------------------------------------\n");
        
        if (results->total_reqs) printf("HTTP Status codes:\n");        
        if (results->respcodes[STATUS_10x])
            printf("  1xx Informational: %d\n", results->respcodes[STATUS_10x]);
        if (results->respcodes[STATUS_20x]) 
            printf("  2xx Success:       %d\n", results->respcodes[STATUS_20x]);
        if (results->respcodes[STATUS_30x]) 
            printf("  3xx Redirection:   %d\n", results->respcodes[STATUS_30x]);
        if (results->respcodes[STATUS_40x]) 
            printf("  4xx Client Error:  %d\n", results->respcodes[STATUS_40x]);
        if (results->respcodes[STATUS_50x]) 
            printf("  5xx Server Error:  %d\n", results->respcodes[STATUS_50x]);

        printf("\n");
    }

    rm_results_free(results);

    return 0;
}
/* main() }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
