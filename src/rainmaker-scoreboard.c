/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <glib.h>

#include "rainmaker-scoreboard.h"

rmScoreboard *rm_scoreboard_new()
{
    rmScoreboard *sb;

    sb = g_malloc0(sizeof(rmScoreboard));
    sb->stopwatch = g_timer_new();

    return sb;
}

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

void rm_scoreboard_free(rmScoreboard *sb)
{
    g_timer_destroy(sb->stopwatch);
    g_free(sb);
}
