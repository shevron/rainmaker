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

void rm_client_run(rmClient *client)
{
    SoupSession *session;
    SoupMessage *msg;
    guint        status;
    int          i;
    GTimer      *timer = g_timer_new();
    rmHeader     *header;

    g_assert(globals->method != NULL);
    g_assert(globals->url    != NULL);

    session = soup_session_sync_new();

#ifdef HAVE_LIBSOUP_COOKIEJAR
    SoupCookieJar *jar = soup_cookie_jar_new();
    soup_session_add_feature(session, (SoupSessionFeature *) jar);
#endif

    msg = soup_message_new_from_uri(globals->method, globals->url);
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);

    if (globals->body != NULL) {
        g_assert(globals->ctype != NULL);
        soup_message_set_request(msg, globals->ctype, SOUP_MEMORY_STATIC, 
            globals->body, globals->bodysize);
    }

    for (header = globals->headers; header != NULL; header = header->next) {
        soup_message_headers_append(msg->request_headers, header->name, header->value);
    }

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

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=syntax 
 */

