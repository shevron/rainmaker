/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-client.h"

/* {{{ rmScoreboard *rm_scoreboard_new()
 *
 * Create a new scoreboard struct
 */
rmScoreboard *rm_scoreboard_new()
{
    rmScoreboard *sb; 

    sb = g_malloc0(sizeof(rmScoreboard));
    sb->stopwatch = g_timer_new();

    return sb;
}
/* rm_scoreboard_new }}} */

/* {{{ void rm_scoreboard_merge(rmScoreboard *target, rmScoreboard *src)
 *
 * Merge results from one scoreboard to another
 */
void rm_scoreboard_merge(rmScoreboard *target, rmScoreboard *src)
{
    gint i;

    g_assert(target != NULL);
    g_assert(src != NULL);

    if (src->requests) { 
        target->requests += src->requests;
        target->elapsed  += src->elapsed;

        for (i = 0; i < 6; i++) { 
            target->resp_codes[i] += src->resp_codes[i];
        }

        target->failed = (target->failed || src->failed);
    }
}
/* rm_scoreboard_merge }}} */

/* {{{ void rm_scoreboard_free(rmScoreboard *sb)
 *
 * Free a scoreboard struct and all related memory
 */
void rm_scoreboard_free(rmScoreboard *sb)
{
    g_timer_destroy(sb->stopwatch);
    g_free(sb);
}
/* rm_scoreboard_free }}} */

/* {{{ rmClient *rm_client_new()
 *
 * Create a new rmClient struct 
 */
rmClient* rm_client_new()
{
    rmClient *client;

    client = g_malloc(sizeof(rmClient));
    client->session    = soup_session_sync_new();
    client->scoreboard = rm_scoreboard_new();

    return client;
}
/* rm_client_new }}} */

/* {{{ void rm_client_set_logger(rmClient *client, SoupLogger *logger)
 *
 * Set a logger for this client
 */
void rm_client_set_logger(rmClient *client, SoupLogger *logger)
{
    soup_session_add_feature(client->session, (SoupSessionFeature *) logger);
}
/* rm_client_set_logger }}} */

/* {{{ void rm_client_free(rmClient *client)
 *
 * Free an rmClient struct and all related resources
 */
void rm_client_free(rmClient *client)
{
    g_object_unref((gpointer) client->session);
    rm_scoreboard_free(client->scoreboard);
    g_free(client);
}
/* rm_client_free }}} */

static void add_header_to_message(rmHeader *header, SoupMessage *msg)
{
    if (header->replace) {
        soup_message_headers_replace(msg->request_headers, header->name, header->value);
    } else {
        soup_message_headers_append(msg->request_headers, header->name, header->value);
    }
}

/* {{{ gboolean rm_client_send_request(rmClient *client, rmRequest *request)
 *
 * Send a single request through the client, keeping score using the scoreboard
 */
guint rm_client_send_request(rmClient *client, rmRequest *request)
{
    SoupMessage *msg;
    guint        status;

    msg = soup_message_new_from_uri(request->method, request->url);
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);

    // Add headers
    g_slist_foreach(request->headers, (GFunc) add_header_to_message, (gpointer) msg);

    // Start timer
    g_timer_start(client->scoreboard->stopwatch);

    // Send request
    status = soup_session_send_message(client->session, msg);

    // Stop timer
    g_timer_stop(client->scoreboard->stopwatch);

    // Count request and response code, add elapsed time
    client->scoreboard->requests++;
    client->scoreboard->resp_codes[(status / 100)]++;
    client->scoreboard->elapsed += g_timer_elapsed(client->scoreboard->stopwatch, NULL);

    g_object_unref((gpointer) msg);

    return status;
}
/* rm_client_send_request }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
