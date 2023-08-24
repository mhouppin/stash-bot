/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TIMEMAN_H
#define TIMEMAN_H

#include "board.h"
#include "uci.h"
#include <sys/timeb.h>
#include <time.h>

// Returns the current time in milliseconds.
INLINED clock_t chess_clock(void)
{
#if defined(_WIN32) || defined(_WIN64)
    struct timeb tp;

    ftime(&tp);
    return (clock_t)tp.time * 1000 + tp.millitm;
#else
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    return (clock_t)tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
#endif
}

// Enum for the type of bestmove
typedef enum bestmove_type_e
{
    NO_BM_TYPE = -1,
    OneLegalMove,
    Promotion,
    SoundCapture,
    SoundCheck,
    Capture,
    Quiet,
    WeirdCheck,
    WeirdQuiet,
    BM_TYPE_NB
} bestmove_type_t;

// Global for scaling time usage based on bestmove type
extern const double BestmoveTypeScale[BM_TYPE_NB];

// Global for scaling time usage based on stability
extern const double BestmoveStabilityScale[5];

// Enum for the type of time management to use
typedef enum timeman_mode_e
{
    Tournament,
    Movetime,
    NoTimeman
} timeman_mode_t;

// Struct for time management
typedef struct _Timeman
{
    clock_t start;
    timeman_mode_t mode;
    bool pondering;
    int checkFrequency;

    clock_t averageTime;
    clock_t maximalTime;
    clock_t optimalTime;

    score_t prevScore;
    move_t prevBestmove;
    move_t prevCountermove;
    int stability;
    bestmove_type_t type;
} Timeman;

// Global for time management
extern Timeman SearchTimeman;

// Initializes the time management based on "go" command parameters.
void timeman_init(const Board *board, Timeman *tm, SearchParams *params, clock_t start);

// Updates the time management based on the current bestmove and score.
void timeman_update(
    Timeman *tm, const Board *board, move_t bestmove, move_t countermove, score_t score);

// Checks time usage periodically.
void check_time(void);

// Checks if we can safely stop the search.
INLINED bool timeman_can_stop_search(Timeman *tm, clock_t cur)
{
    if (tm->pondering && SearchWorkerPool.ponder) return false;
    return tm->mode != NoTimeman && cur >= tm->start + tm->optimalTime;
}

// Checks if we must stop the search.
INLINED bool timeman_must_stop_search(Timeman *tm, clock_t cur)
{
    if (tm->pondering && SearchWorkerPool.ponder) return false;
    return tm->mode != NoTimeman && cur >= tm->start + tm->maximalTime;
}

#endif
