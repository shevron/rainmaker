#include <glib.h>
#include <libsoup/soup.h>

#ifndef _HAVE_RAINMAKER_CLIENT_H

typedef struct _rmScoreBoard { 
    guint   requests;
    guint   resp_codes[5];
    gdouble elapsed_s;
    gulong  elapsed_u;
    GTimer *stopwatch;
} rmScoreBoard; 

typedef struct _rmClient {
    SoupSession  *session;
    rmScoreBoard *scoreboard;
} rmClient;

rmScoreBoard *rm_scoreboard_new();
void          rm_scoreboard_merge(rmScoreboard *target, rmScoreboard *src);
void          rm_scoreboard_free(rmScoreboard *sb);
rmClient*     rm_client_new();
void          rm_client_free(rmClient *client);
gboolean      rm_client_send_request(rmClient *client, rmRequest *request);

#define _HAVE_RAINMAKER_CLIENT_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
