/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-client.h"
#include "rainmaker-scenario.h"
#include "rainmaker-scoreboard.h"

/// Create a new client and allocate relevant memory. Will also allocate
/// the client's SoupSession and rmScoreboard
rmClient* rm_client_new()
{
    rmClient *client;

    client = g_malloc(sizeof(rmClient));
    client->session    = soup_session_sync_new();
    client->scoreboard = rm_scoreboard_new();

    return client;
}

/// Set the client's logger object
void rm_client_set_logger(rmClient *client, SoupLogger *logger)
{
    soup_session_add_feature(client->session, (SoupSessionFeature *) logger);
}

/// Free a client and related memory. Will also unref the client's SoupSession
/// and free the associated scoreboard
void rm_client_free(rmClient *client)
{
    g_object_unref((gpointer) client->session);
    rm_scoreboard_free(client->scoreboard);
    g_free(client);
}

/// Add a header struct to a SoupMessage. If the header's replace flag is
/// set, will replace any existing headers with the same name.
static void add_header_to_message(rmHeader *header, SoupMessage *msg)
{
    if (header->replace) {
        soup_message_headers_replace(msg->request_headers, header->name, header->value);
    } else {
        soup_message_headers_append(msg->request_headers, header->name, header->value);
    }
}

/// Send a request. Will convert the rmRequest struct to a SoupMessage and send
/// it synchronously.
///
/// @todo consider re-using the message objects for performance reasons
static guint rm_client_send_request(rmClient *client, rmRequest *request)
{
    SoupMessage *msg;
    guint        status, s;

    msg = soup_message_new_from_uri(g_quark_to_string(request->method), request->url);
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);

    // Add body
    if (request->body != NULL) {
        g_assert(request->bodyType);
        soup_message_set_request(msg, g_quark_to_string(request->bodyType),
                SOUP_MEMORY_TEMPORARY, request->body, request->bodyLength);
    }

    // Add headers
    g_slist_foreach(request->headers, (GFunc) add_header_to_message, (gpointer) msg);

    // Start timer
    g_timer_start(client->scoreboard->stopwatch);

    // Send request
    status = soup_session_send_message(client->session, msg);
    s = status / 100;

    // Stop timer
    g_timer_stop(client->scoreboard->stopwatch);

    // Count request and response code, add elapsed time
    client->scoreboard->requests++;
    if (s >= 0 && s <= 5) {
        client->scoreboard->resp_codes[s]++;
    } else {
        g_printerr("WARNING: unexpected HTTP response code '%u' encountered\n", status);
    }
    client->scoreboard->elapsed += g_timer_elapsed(client->scoreboard->stopwatch, NULL);

    g_object_unref((gpointer) msg);

    return status;
}

/// Run a scenario using the client.
void rm_client_run_scenario(rmClient *client, rmScenario *scenario)
{
    GSList        *rlNode;
    guint          status, i;
    SoupCookieJar *cookieJar = NULL;

    // Enable cookie persistence if needed
    if (scenario->persistCookies) {
        cookieJar = soup_cookie_jar_new();
        soup_session_add_feature(client->session, (SoupSessionFeature *) cookieJar);
    }

    for (rlNode = scenario->requests; rlNode; rlNode = rlNode->next) {
        rmRequest *req = (rmRequest *) rlNode->data;
        g_assert(req != NULL);
        g_assert(req->repeat >= 1); // request is sane

        for (i = 0; i < req->repeat; i++) {
            status = rm_client_send_request(client, req);

            if ((scenario->failOnTcpError && status < 100) ||
                (scenario->failOnHttpRedirect && status >= 300 && status < 400) ||
                (scenario->failOnHttpError && status >= 400)) {

                client->scoreboard->failed = TRUE;
                break;
            }
        }

        if (client->scoreboard->failed) break;
    }

    if (cookieJar) g_object_unref(cookieJar);
}

// vim:ts=4:expandtab:cindent:sw=2
