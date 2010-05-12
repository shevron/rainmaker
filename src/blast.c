#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>

int main(int argc, char *argv[])
{
	SoupSession *session;
	SoupLogger  *logger;
	SoupMessage *msg;
	guint        status;

	g_type_init();
	g_thread_init(NULL);

	logger = soup_logger_new(SOUP_LOGGER_LOG_HEADERS, -1);

	session = soup_session_sync_new();
	soup_logger_attach(logger, session);
	g_object_unref(logger);

	msg = soup_message_new("GET", "http://example.com");
	soup_message_headers_append(
		msg->request_headers, 
		"Accept", 
		"text/xml, *;q=0.5"
	);

	status = soup_session_send_message(session, msg);

	g_print("HTTP status code is %d, got %d bytes in response body\n",
		status, (int) msg->response_body->length);

	g_free(session);

	return 0;
}
