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

char const *methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};

const gchar *ctype_form_urlencoded = "application/x-www-form-urlencoded";
const gchar *ctype_octetstream     = "application/octet-stream";

static void rm_globals_init()
{
	globals = g_malloc(sizeof(rmGlobals));

	globals->tcmutex  = g_mutex_new();
	globals->body     = NULL;
	globals->freebody = FALSE;
	globals->ctype    = NULL;
	globals->url      = NULL;
}

static void rm_globals_destroy()
{
	if (globals->url != NULL) soup_uri_free(globals->url);
	if (globals->freebody) g_free(globals->body);
	g_mutex_free(globals->tcmutex);

	g_free(globals);
}

static rmClient *rm_client_init()
{
	rmClient *client;

	client = g_malloc(sizeof(rmClient));
	client->status_10x = 0;
	client->status_20x = 0;
	client->status_30x = 0;
	client->status_40x = 0;
	client->status_50x = 0;
	client->timer      = 0;

	return client;
}

static void rm_client_destroy(rmClient *client)
{
	g_free(client);
}

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

	g_assert(globals != NULL);

	/* Define and parse command line arguments */
	GOptionEntry    options[] = {
		{"clients",  'c', 0, G_OPTION_ARG_INT, &clients, 
			"number of concurrent clients to run", NULL},
		{"requests", 'r', 0, G_OPTION_ARG_INT, &requests, 
			"number of requests to send per client", NULL},
		{"postdata", 'p', 0, G_OPTION_ARG_STRING, &postdata, 
			"post data (will send a POST request)", NULL},
		{"postfile", 0,   0, G_OPTION_ARG_FILENAME, &postfile, 
			"read body from file and send as a POST request", NULL},
		{"putfile",  0,   0, G_OPTION_ARG_FILENAME, &putfile,
			"read body from file and send as a PUT request", NULL},
		{"ctype",    't', 0, G_OPTION_ARG_STRING, &ctype, 
			"content type to use in POST/PUT requests", NULL},
 
		{ NULL }
	};

	context = g_option_context_new("<target URL>");
	g_option_context_set_summary(context, 
		PACKAGE_NAME " - a simple HTTP load generator, version " PACKAGE_VERSION);
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, options, NULL);

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

	if (postdata != NULL) {
		g_assert(postfile == NULL);
		g_assert(putfile  == NULL);

		globals->method   = methods[METHOD_POST];
		globals->body     = postdata;
		globals->bodysize = (gsize) strlen(postdata);
		globals->ctype    = (gchar *) ctype_form_urlencoded;
	}

	if (postfile != NULL) {
		g_assert(postdata == NULL);
		g_assert(putfile  == NULL);

		globals->method   = methods[METHOD_POST];
		globals->ctype    = (gchar *) ctype_form_urlencoded;
		if (! g_file_get_contents(postfile, &globals->body, &globals->bodysize, &error)) {
			g_printerr("Error loading body: %s\n", error->message);
			return FALSE;
		}
		globals->freebody = TRUE;
	}

	if (putfile != NULL) {
		g_assert(postdata == NULL);
		g_assert(postfile == NULL);

		globals->method   = methods[METHOD_PUT];
		globals->ctype    = (gchar *) ctype_octetstream;
		if (! g_file_get_contents(putfile, &globals->body, &globals->bodysize, &error)) {
			g_printerr("error loading body: %s\n", error->message);
			return FALSE;
		}

		globals->freebody = TRUE;
	}
	
	if (globals->body != NULL && ctype != NULL) {
		globals->ctype = ctype;
	}

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

	if ((globals->url = soup_uri_new(argv[1])) == NULL) {
		g_printerr("Error: unable to parse URL '%s'\n", argv[1]);
		return FALSE;
	}

	return TRUE;
}

int main(int argc, char *argv[])
{
	GTimer    *timer;
	rmClient **clients;
	int        status_10x  = 0;
	int        status_20x  = 0;
	int        status_30x  = 0;
	int        status_40x  = 0;
	int        status_50x  = 0;
	int        total_reqs  = 0;
	double     total_time  = 0;
	double     avg_reqtime = 0;
	int        i;

	g_type_init();
	g_thread_init(NULL);

	rm_globals_init();

	if (! parse_load_cmd_args(argc, argv)) {
		exit(1);
	}

	globals->tcount = globals->clients; 
	clients = g_malloc(sizeof(rmClient*) * globals->clients); 
	timer = g_timer_new();

	g_timer_start(timer);
	for (i = 0; i < globals->clients; i++) {
		clients[i] = rm_client_init();
		g_thread_create((GThreadFunc) rm_client_run, clients[i], FALSE, NULL);
	}

	while(globals->tcount > 0) g_usleep(10000); 
	g_timer_stop(timer);

	for (i = 0; i < globals->clients; i++) {
		status_10x += clients[i]->status_10x;
		status_20x += clients[i]->status_20x;
		status_30x += clients[i]->status_30x;
		status_40x += clients[i]->status_40x;
		status_50x += clients[i]->status_50x;
		avg_reqtime += clients[i]->timer;

 		rm_client_destroy(clients[i]);
	}
	g_free(clients);

	total_reqs = status_10x + status_20x + status_30x + status_40x + status_50x;
	total_time = g_timer_elapsed(timer, NULL);
	avg_reqtime /= globals->clients;

	/* Print out summary */
	printf("--------------------------------------------------------------\n");
	printf("Completed %d requests in %.3f seconds\n", total_reqs, total_time);
	printf("  %10.3f avg. requests per second accross all clients\n", 
		(total_reqs / total_time));
	printf("  %10.3f avg. requests per second per client\n", (1 / avg_reqtime));
	printf("--------------------------------------------------------------\n");
	printf("HTTP Status codes:\n");
	printf("  1xx Informational: %d\n", status_10x);
	printf("  2xx Success:       %d\n", status_20x);
	printf("  3xx Redirection:   %d\n", status_30x);
	printf("  4xx Client Error:  %d\n", status_40x);
	printf("  5xx Server Error:  %d\n", status_50x);
	printf("\n");

	g_timer_destroy(timer);
	rm_globals_destroy();

	return 0;
}

