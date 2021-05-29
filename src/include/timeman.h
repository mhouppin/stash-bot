/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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
# define TIMEMAN_H

# include <time.h>
# include <sys/timeb.h>
# include "board.h"
# include "uci.h"

INLINED clock_t chess_clock(void)
{
#if defined(_WIN32) || defined(_WIN64)
    struct timeb    tp;

    ftime(&tp);
    return ((clock_t)tp.time * 1000 + tp.millitm);
#else
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    return ((clock_t)tp.tv_sec * 1000 + tp.tv_nsec / 1000000);
#endif
}

typedef enum
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
}
bestmove_type_t;

extern const double BestmoveTypeScale[BM_TYPE_NB];
extern const double BestmoveStabilityScale[5];

typedef enum tm_mode_e
{
    Tournament,
    Movetime,
    NoTimeman
}
tm_mode_t;

typedef struct timeman_s
{
    clock_t start;
    tm_mode_t mode;
    bool pondering;

    clock_t averageTime;
    clock_t maximalTime;
    clock_t optimalTime;

    score_t prevScore;
    move_t prevBestmove;
    int stability;
    bestmove_type_t type;
}
timeman_t;

extern timeman_t Timeman;

void timeman_init(const board_t *board, timeman_t *tm, goparams_t *params, clock_t start);
void timeman_update(timeman_t *tm, const board_t *board, move_t bestmove, score_t score);
void check_time(void);

INLINED bool timeman_can_stop_search(timeman_t *tm, clock_t cur)
{
    if (tm->pondering && EnginePonderhit == 0)
        return (false);
    return (tm->mode != NoTimeman && cur >= tm->start + tm->optimalTime);
}

INLINED bool timeman_must_stop_search(timeman_t *tm, clock_t cur)
{
    if (tm->pondering && EnginePonderhit == 0)
        return (false);
    return (tm->mode != NoTimeman && cur >= tm->start + tm->maximalTime);
}

#endif
