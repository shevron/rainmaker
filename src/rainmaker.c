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

/* {{{ rm_request_new()
 *
 */
rmRequest *rm_request_new()
{
    rmRequest *request;

    request = g_malloc(sizeof(rmRequest));

    request->method    = NULL;
    request->url       = NULL;
    request->useragent = NULL;
    request->headers   = NULL;
    request->ctype     = NULL;
    request->body      = NULL;
    request->bodysize  = 0;
    request->freebody  = FALSE;

    return request;
}
/* rm_request_new() }}} */

/* {{{ rm_request_free 
 *
 */
void rm_request_free(rmRequest *request)
{
    if (request->url != NULL) 
        soup_uri_free(request->url);
    if (request->useragent != NULL) 
        g_free(request->useragent);
    if (request->freebody) 
        g_free(request->body);

    rm_header_free_all(request->headers);

    g_free(request);
}
/* rm_request_free() }}} */

/* {{{ rm_header_new() - create a new rmHeader struct. 
 *
 * The header's name and value are owned by the caller, and will not be
 * copied, modified or freed.
 */
rmHeader *rm_header_new(gchar *name, gchar *value)
{
    rmHeader *header = g_malloc(sizeof(rmHeader));

    header->next = NULL;
    header->name = name;
    header->value = value;

    return header;
}
/* rm_header_new() }}} */

/* {{{ rm_header_free_all() - free a linked-list of header structs 
 *
 */
void rm_header_free_all(rmHeader *header)
{
    rmHeader *next = NULL;

    while (header != NULL) {
        next = header->next;
        g_free(header);
        header = next;
    }
}
/* rm_header_free_all() }}} */

/* {{{ rm_results_new() 
 *
 */
rmResults *rm_results_new()
{
    rmResults *results = g_malloc(sizeof(rmResults));

    results->clients    = 0;
    results->total_reqs = 0;
    results->total_time = 0;
    memset(results->respcodes, 0, sizeof(guint) * 5);

    return results;
}
/* rm_results_new() }}} */

/* {{{ rm_results_add() 
 *
 * Add a set of results up to another set of results
 */
rmResults *rm_results_add(rmResults *total, rmResults *add)
{
    gint i;

    g_assert(total != NULL);
    g_assert(add   != NULL);

    total->clients    += add->clients;
    total->total_reqs += add->total_reqs;
    total->total_time += add->total_time;
    for (i = 0; i < 4; i++) {
        total->respcodes[i] += add->respcodes[i];
    }

    return total;
}
/* rm_results_add() }}} */

/* {{{ rm_results_free()
 *
 */
void rm_results_free(rmResults *results)
{
    g_free(results);
}
/* rm_results_free() }}} */

/* {{{ rm_client_init() - Initialize an rmClient struct 
 *
 */
rmClient *rm_client_init(guint repeats, rmRequest *request)
{
    rmClient *client;

    client = g_malloc(sizeof(rmClient));
    client->repeats    = repeats;
    client->request    = request;
    client->done       = FALSE;
    client->error      = NULL;
    
    client->results          = rm_results_new();
    client->results->clients = 1;

    return client;
}
/* rm_client_init() }}} */

/* {{{ rm_client_free() - free an rmClient struct 
 *
 */
void rm_client_free(rmClient *client)
{
    rm_results_free(client->results);
    g_free(client);
}
/* rm_client_free() }}} */

/* {{{ rm_client_run() - run a load-generating client 
 *
 * This is where the magic really happens :)
 */
void rm_client_run(rmClient *client)
{
    SoupSession *session;
    SoupMessage *msg;
    rmRequest   *request;
    guint        status;
    int          i;
    GTimer      *timer = g_timer_new();
    rmHeader    *header;

    request = client->request;

    g_assert(request->method != NULL);
    g_assert(request->url    != NULL);

    session = soup_session_sync_new();

    /* Set user agent string */
    if (request->useragent != NULL) {
        GValue *useragent = g_malloc0(sizeof(GValue));

        g_value_init(useragent, G_TYPE_STRING);
        g_value_set_string(useragent, request->useragent);
        g_object_set_property((GObject *) session, SOUP_SESSION_USER_AGENT, 
            useragent);

        g_value_unset(useragent);
        g_free(useragent);
    }

#ifdef HAVE_LIBSOUP_COOKIEJAR
    if (request->savecookies) {
        SoupCookieJar *jar = soup_cookie_jar_new();
        soup_session_add_feature(session, (SoupSessionFeature *) jar);
    }
#endif

    msg = soup_message_new_from_uri(request->method, request->url);
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);

    if (request->body != NULL) {
        g_assert(request->ctype != NULL);
        soup_message_set_request(msg, request->ctype, SOUP_MEMORY_STATIC, 
            request->body, request->bodysize);
    }

    for (header = request->headers; header != NULL; header = header->next) {
        soup_message_headers_append(msg->request_headers, header->name, header->value);
    }

    for (i = 0; i < client->repeats; i++) {
        g_timer_start(timer);
        status = soup_session_send_message(session, msg);
        g_timer_stop(timer);

        if (SOUP_STATUS_IS_TRANSPORT_ERROR(status)) {
            client->error = g_error_new_literal(SOUP_HTTP_ERROR, status, msg->reason_phrase);
            break;
        }

        if (client->stoponerror && (status >= 400 && status <= 599)) {
            client->error = g_error_new(SOUP_HTTP_ERROR, status, "got HTTP %d (%s) response from server, aborting",
                status, msg->reason_phrase);
            break;
        }
        
        g_assert(status >= 100 && status <= 599);
        
        client->results->respcodes[(status / 100) - 1]++;
        client->results->total_reqs++;
        client->results->total_time += g_timer_elapsed(timer, NULL);
    }
        
    g_timer_destroy(timer);

    client->done = TRUE;
}
/* rm_client_run }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker 
 */

