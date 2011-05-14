/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#ifndef RAINMAKER_SCOREBOARD_H_
#define RAINMAKER_SCOREBOARD_H_

#include <glib.h>

typedef struct _rmScoreboard {
    guint     requests;
    guint     resp_codes[6];
    gdouble   elapsed;
    GTimer   *stopwatch;
    gboolean  failed;
} rmScoreboard;

rmScoreboard* rm_scoreboard_new();
void          rm_scoreboard_merge(rmScoreboard *target, rmScoreboard *src);
void          rm_scoreboard_free(rmScoreboard *sb);

#endif // RAINMAKER_SCOREBOARD_H_

// vim:ts=4:expandtab:cindent:sw=2
