#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>

#include "config.h"

SoupLogger    *logger;
int            reqs;
gchar         *url;
const char    *method  = "GET";
static GMutex *tcmutex = NULL;
int            tcount  = 0;

static void blast_worker()
{
	SoupSession *session;
	SoupMessage *msg;
	guint        status;
	int          i;

	session = soup_session_sync_new();
	soup_logger_attach(logger, session);

	msg = soup_message_new(method, url);
	if (! msg) {
		g_printerr("Error: unable to parse URL '%s'\n", url);

	} else {

		/*
		soup_message_headers_append(
			msg->request_headers, 
			"Accept", 
			"text/xml, *;q=0.5"
		);
		*/

		for (i = 0; i < reqs; i++) {
			status = soup_session_send_message(session, msg);
		}
	
		//g_free(session);
	}

	g_mutex_lock(tcmutex);
	tcount--;
	g_mutex_unlock(tcmutex);
}

int main(int argc, char *argv[])
{
	int             i, workers = 1;
	GError         *error = NULL;
	GOptionContext *context;

	GOptionEntry    options[] = {
		{"clients", 'c', 0, G_OPTION_ARG_INT, &workers, "number of concurrent clients to run", NULL},
		{"requests", 'r', 0, G_OPTION_ARG_INT, &reqs, "number of requests to send per client", NULL},
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
		
	url = (gchar *) argv[1];
	if (reqs < 1) reqs = 1; 

	g_type_init();
	g_thread_init(NULL);

	logger = soup_logger_new(SOUP_LOGGER_LOG_MINIMAL, -1);

	g_assert(tcmutex == NULL);
	tcmutex = g_mutex_new();

	tcount = workers; 

	for (i = 0; i < workers; i++) {
		g_thread_create((GThreadFunc) blast_worker, NULL, FALSE, NULL);
	}

	while(tcount > 0) g_usleep(10000); 

	return 0;
}
