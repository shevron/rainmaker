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
    sb->stopwatch = NULL;

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

    target->requests += src->requests;
    target->elapsed_s += src->elapsed_s;
    target->elapsed_u += src->elapsed_u;

    for (i = 0; i < 5; i++) { 
        target->resp_codes[i] += src->resp_codes[i];
    }
}
/* rm_scoreboard_merge }}} */

/* {{{ void rm_scoreboard_free(rmScoreboard *sb)
 *
 * Free a scoreboard struct and all related memory
 */
void rm_scoreboard_free(rmScoreboard *sb)
{
    if (sb->stopwatch != NULL) g_timer_destroy(sb->stopwatch);
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

/* {{{ void rm_client_free(rmClient *client)
 *
 * Free an rmClient struct and all related resources
 */
void rm_client_free(rmClient *client)
{
    g_free(client->session);
    rm_scoreboard_free(client->scoreboard);
    g_free(client);
}
/* rm_client_free }}} */

/* {{{ gboolean rm_client_send_request(rmClient *client, rmRequest *request)
 *
 * Send a single request through the client, keeping score using the scoreboard
 */
gboolean rm_client_send_request(rmClient *client, rmRequest *request)
{
    SoupMessage *msg;
    guint        status;
    gulong       elapsed_u;

    msg = soup_message_new_from_uri(request->method, request->url);

    // Start timer
    client->scoreboard->stopwatch = g_timer_new();

    // Send request
    status = soup_session_send_message(client->session, msg);

    // Stop timer
    g_timer_stop(client->scoreboard->stopwatch);

    // Count request and response code, add elapsed time
    client->scoreboard->requests++;
    client->scoreboard->resp_codes[(status / 100) - 1]++;
    client->scoreboard->elapsed_s += g_timer_elapsed(client->scoreboard->stopwatch, &elapsed_u);
    client->scoreboard->elapsed_u += elapsed_u;

    return TRUE;
}
/* rm_client_send_request }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
