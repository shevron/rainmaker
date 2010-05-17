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

typedef enum {
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_HEAD
} rmMethod;

enum {
  STATUS_10x,
  STATUS_20x,
  STATUS_30x,
  STATUS_40x,
  STATUS_50x,
};

char const *methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};

const gchar *ctype_form_urlencoded = "application/x-www-form-urlencoded";
const gchar *ctype_octetstream     = "application/octet-stream";

static void rm_headers_free_all(rmHeader *header);

/* {{{ rm_globals_init() - Initialize global variables struct
 *
 */
static void rm_globals_init()
{
    globals = g_malloc(sizeof(rmGlobals));

    globals->useragent = NULL;
    globals->headers   = NULL;
    globals->body      = NULL;
    globals->bodysize  = 0;
    globals->freebody  = FALSE;
    globals->ctype     = NULL;
    globals->url       = NULL;
    globals->method    = NULL;
}
/* rm_globals_init() }}} */

/* {{{ rm_globals_destroy() - Clean up global variables 
 *
 */
static void rm_globals_destroy()
{
    if (globals->url != NULL) soup_uri_free(globals->url);
    if (globals->useragent != NULL) g_free(globals->useragent);
    if (globals->freebody) g_free(globals->body);
    rm_headers_free_all(globals->headers);

    g_free(globals);
}
/* rm_globals_destroy() }}} */

/* {{{ rm_client_init() - Initialize an rmClient struct 
 *
 */
static rmClient *rm_client_init()
{
    rmClient *client;

    client = g_malloc(sizeof(rmClient));
    memset(client->statuses, 0, sizeof(guint) * 5);
    client->total_reqs = 0;
    client->done       = FALSE;
    client->timer      = 0;
    client->error      = NULL;

    return client;
}
/* rm_client_init() }}} */

/* {{{ rm_client_destroy() - free an rmClient struct 
 *
 */
static void rm_client_destroy(rmClient *client)
{
    if (client->error != NULL) g_error_free(client->error);
    g_free(client);
}
/* rm_client_destroy() }}} */

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

    header = g_malloc(sizeof(rmHeader));
    header->next = NULL;

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

    header->name = name;
    header->value = value;

    return header;
}
/* rm_header_new_from_string() }}} */

/* {{{ rm_headers_free_all() - free a linked-list of header structs 
 *
 */
static void rm_headers_free_all(rmHeader *header)
{
    rmHeader *next = NULL;

    while (header != NULL) {
        next = header->next;
        g_free(header);
        header = next;
    }
}
/* rm_headers_free_all() }}} */

/* {{{ parse_load_cmd_args() - parse command line arguments and load config
 *
 */
static gboolean parse_load_cmd_args(int argc, char *argv[])
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

    g_assert(globals != NULL);

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

    if ((postdata != NULL && (postfile != NULL || putfile != NULL)) ||
        (postfile != NULL && putfile != NULL)) {
        
        g_printerr("error: you may only specify one of --postdata, --postfile and --putfile\n");
        return FALSE;
    }

    /* Set method and body */
    if (postdata != NULL) {
        g_assert(postfile == NULL);
        g_assert(putfile  == NULL);

        globals->method   = methods[METHOD_POST];
        globals->body     = postdata;
        globals->bodysize = (gsize) strlen(postdata);
        globals->ctype    = (gchar *) ctype_form_urlencoded;

    } else if (postfile != NULL) {
        g_assert(postdata == NULL);
        g_assert(putfile  == NULL);

        globals->method   = methods[METHOD_POST];
        globals->ctype    = (gchar *) ctype_form_urlencoded;
        if (! g_file_get_contents(postfile, &globals->body, &globals->bodysize, &error)) {
            g_printerr("Error loading body: %s\n", error->message);
            return FALSE;
        }
        globals->freebody = TRUE;

    } else if (putfile != NULL) {
        g_assert(postdata == NULL);
        g_assert(postfile == NULL);

        globals->method   = methods[METHOD_PUT];
        globals->ctype    = (gchar *) ctype_octetstream;
        if (! g_file_get_contents(putfile, &globals->body, &globals->bodysize, &error)) {
            g_printerr("error loading body: %s\n", error->message);
            return FALSE;
        }

        globals->freebody = TRUE;

    } else {
        globals->method = methods[METHOD_GET];
    }
    
    /* Set content type */
    if (globals->body != NULL && ctype != NULL) {
        globals->ctype = ctype;
    }

    /* Set user agent */
    /* TODO: make this a command line argument / config option */
    globals->useragent = g_strdup(RAINMAKER_USERAGENT);
    
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

            if (globals->headers == NULL) globals->headers = header;
            if (prev != NULL) prev->next = header;
            prev = header;
        }
    }

    /* Set number of requests / clients */
    if (requests <= 1) {
        globals->requests = 1; 
    } else {
        globals->requests = requests;
    }

    if (clients <= 1) {
        globals->clients = 1;
    } else {
        globals->clients = clients;
    }

    /* Set URL */
    if ((globals->url = soup_uri_new(argv[1])) == NULL) {
        g_printerr("Error: unable to parse URL '%s'\n", argv[1]);
        return FALSE;
    }

#ifdef HAVE_LIBSOUP_COOKIEJAR
    globals->savecookies = savecookies;
#endif

    return TRUE;
}
/* parse_load_cmd_args() }}} */

/* {{{ rm_control_run() - run the control thread 
 *
 */
static void rm_control_run(rmClient **clients)
{
    int       i, total_reqs, done_reqs = 0; 
    gboolean  done;
    gdouble   total_time;
    gdouble   current_load = 0;

    g_assert(clients != NULL);

    total_reqs = globals->clients * globals->requests;

    do {
        g_usleep(50000);
        done = TRUE;
        done_reqs = 0;
        total_time = 0;

        for (i = 0; clients[i] != NULL; i++) {
            done = (done && clients[i]->done);
            done_reqs += clients[i]->total_reqs;
            total_time += clients[i]->timer;
        }
        
        if (total_time && globals->clients) {
            current_load = done_reqs / (total_time / globals->clients);
        } 
        printf("[Sent %d/%d requests, running at %.3f req/sec]%10s", 
            done_reqs, total_reqs, current_load, "\r");
        fflush(stdout);

    } while (! done);

    printf("\n");
}
/* rm_control_run() }}} */

/* {{{ main() - what do you think?
 *
 */
int main(int argc, char *argv[])
{
    rmClient **clients;
    GThread   *ctl_thread;
    int        status_10x  = 0;
    int        status_20x  = 0;
    int        status_30x  = 0;
    int        status_40x  = 0;
    int        status_50x  = 0;
    int        total_reqs  = 0;
    double     total_time  = 0;
    double     avg_load_client = 0;
    double     avg_load_server = 0;
    int        i;

    g_type_init();
    g_thread_init(NULL);

    rm_globals_init();

    if (! parse_load_cmd_args(argc, argv)) {
        exit(1);
    }

    clients = g_malloc(sizeof(rmClient*) * (globals->clients + 1)); 

    for (i = 0; i < globals->clients; i++) {
        clients[i] = rm_client_init();
        g_thread_create((GThreadFunc) rm_client_run, clients[i], FALSE, NULL);
    }
    clients[globals->clients] = NULL;

    ctl_thread = g_thread_create((GThreadFunc) rm_control_run, clients, TRUE, NULL);
    g_thread_join(ctl_thread);

    /* sum up results */
    for (i = 0; i < globals->clients; i++) {
        if (clients[i]->error != NULL) {
            g_printerr("Client error: %s\n", clients[i]->error->message);
            break;
        }
        status_10x += clients[i]->statuses[STATUS_10x];
        status_20x += clients[i]->statuses[STATUS_20x];
        status_30x += clients[i]->statuses[STATUS_30x];
        status_40x += clients[i]->statuses[STATUS_40x];
        status_50x += clients[i]->statuses[STATUS_50x];
        total_reqs += clients[i]->total_reqs;
        total_time += clients[i]->timer;
    }

    /* free clients */
    for (i = 0; i < globals->clients; i++) {
        rm_client_destroy(clients[i]);
    }
    g_free(clients);

    avg_load_client = 1 / (total_time / total_reqs);
    avg_load_server = 1 / ((total_time / globals->clients) / total_reqs);

    /* Print out summary */
    if (total_reqs) {
        printf("--------------------------------------------------------------\n");
        printf("Completed %d requests in %.3f seconds\n", total_reqs, (total_time / globals->clients));
        printf("  Average load on server:  %.3f req/sec\n", avg_load_server);
        printf("  Average load per client: %.3f req/sec\n", avg_load_client);
        printf("--------------------------------------------------------------\n");
        if (total_reqs) printf("HTTP Status codes:\n");
        if (status_10x) printf("  1xx Informational: %d\n", status_10x);
        if (status_20x) printf("  2xx Success:       %d\n", status_20x);
        if (status_30x) printf("  3xx Redirection:   %d\n", status_30x);
        if (status_40x) printf("  4xx Client Error:  %d\n", status_40x);
        if (status_50x) printf("  5xx Server Error:  %d\n", status_50x);
        printf("\n");
    }

    rm_globals_destroy();

    return 0;
}
/* main() }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
