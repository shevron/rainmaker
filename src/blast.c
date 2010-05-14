#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>

#include "config.h"

typedef struct _blastClient {
	int     status_10x;
	int     status_20x;
	int     status_30x;
	int     status_40x;
	int     status_50x;
	double  timer;
} blastClient;

typedef struct _blastGlobals {
	int		     requests;
	int          tcount;
	SoupURI     *url;
	const gchar *method;
	GMutex      *tcmutex;
} blastGlobals;

typedef enum {
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_HEAD
} blastMethod;

char const *methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};

static blastGlobals *globals;

static gint blast_globals_init(gchar *url, blastMethod method)
{
	globals = g_malloc(sizeof(blastGlobals));
	globals->method = methods[method];
	globals->tcmutex = g_mutex_new();
	
	if ((globals->url = soup_uri_new(url)) == NULL) {
		g_printerr("Error: unable to parse URL '%s'\n", url);
		return FALSE;
	}

	return TRUE;
}

static void blast_globals_destroy()
{
	soup_uri_free(globals->url);
	g_mutex_free(globals->tcmutex);
	g_free(globals);
}

static void blast_worker(gpointer data)
{
	SoupSession *session;
	SoupMessage *msg;
	guint        status;
	int          i;
	blastClient *client = (blastClient *) data;
	GTimer      *timer = g_timer_new();

	session = soup_session_sync_new();
	msg = soup_message_new_from_uri(globals->method, globals->url);

	/*
	soup_message_headers_append(
		msg->request_headers, 
		"Accept", 
		"text/xml, *;q=0.5"
	);
	*/

	for (i = 0; i < globals->requests; i++) {
		g_timer_start(timer);
		status = soup_session_send_message(session, msg);
		g_timer_stop(timer);

		switch (status / 100) { 
			case 1: client->status_10x++; break;
			case 2: client->status_20x++; break;
			case 3: client->status_30x++; break;
			case 4: client->status_40x++; break;
			case 5: client->status_50x++; break;
		}

		client->timer += g_timer_elapsed(timer, NULL);
	}
		
	client->timer /= globals->requests;
	g_timer_destroy(timer);

	g_mutex_lock(globals->tcmutex);
	globals->tcount--;
	g_mutex_unlock(globals->tcmutex);
}

static blastClient *blast_client_init()
{
	blastClient *client;

	client = g_malloc(sizeof(blastClient));
	client->status_10x = 0;
	client->status_20x = 0;
	client->status_30x = 0;
	client->status_40x = 0;
	client->status_50x = 0;
	client->timer      = 0;

	return client;
}

static void blast_client_destroy(blastClient *client)
{
	g_free(client);
}

int main(int argc, char *argv[])
{
	int              i, requests, workers = 1;
	GError          *error = NULL;
	GOptionContext  *context;
	GTimer          *timer;
	blastClient    **clients;

	int              status_10x  = 0;
	int              status_20x  = 0;
	int              status_30x  = 0;
	int              status_40x  = 0;
	int              status_50x  = 0;
	int              total_reqs  = 0;
	double           total_time  = 0;
	double           avg_reqtime = 0;

	/* Define and parse command line arguments */
	GOptionEntry    options[] = {
		{"clients", 'c', 0, G_OPTION_ARG_INT, &workers, "number of concurrent clients to run", NULL},
		{"requests", 'r', 0, G_OPTION_ARG_INT, &requests, "number of requests to send per client", NULL},
		{ NULL }
	};

	context = g_option_context_new("<target URL>");
	g_option_context_set_summary(context, 
		PACKAGE_NAME " - a simple HTTP load generator, version " PACKAGE_VERSION);
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, options, NULL);

	if (! g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("command line arguments parsing failed: %s\n",
			error->message);
		exit(1);
	}

	g_option_context_free(context);

	if (argc < 2 || argc > 2) {
		g_printerr("error: no target URL specified, run with --help for help\n");
		exit(1);
	}
	
	g_type_init();
	g_thread_init(NULL);
	
	if (! blast_globals_init((gchar *) argv[1], METHOD_GET)) {
		exit(1);
	}

	if (requests < 1) {
		globals->requests = 1; 
	} else {
		globals->requests = requests;
	}

	globals->tcount = workers; 
	clients = g_malloc(sizeof(blastClient*) * workers); 
	timer = g_timer_new();

	g_timer_start(timer);
	for (i = 0; i < workers; i++) {
		clients[i] = blast_client_init();
		g_thread_create((GThreadFunc) blast_worker, clients[i], FALSE, NULL);
	}

	while(globals->tcount > 0) g_usleep(10000); 
	g_timer_stop(timer);

	for (i = 0; i < workers; i++) {
		status_10x += clients[i]->status_10x;
		status_20x += clients[i]->status_20x;
		status_30x += clients[i]->status_30x;
		status_40x += clients[i]->status_40x;
		status_50x += clients[i]->status_50x;
		avg_reqtime += clients[i]->timer;

 		blast_client_destroy(clients[i]);
	}
	g_free(clients);

	total_reqs = status_10x + status_20x + status_30x + status_40x + status_50x;
	total_time = g_timer_elapsed(timer, NULL);
	avg_reqtime /= workers;

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
	blast_globals_destroy();

	return 0;
}
