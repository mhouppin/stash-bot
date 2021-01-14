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

#include <math.h>
#include <stdlib.h>
#include "imath.h"
#include "timeman.h"

void    timeman_init(const board_t *board, timeman_t *tm,
        goparams_t *params, clock_t start)
{
    extern ucioptions_t g_options;
    clock_t             overhead = g_options.move_overhead;

    tm->start = start;

    if (params->wtime || params->btime)
    {
        tm->mode = Tournament;

        double  mtg = (params->movestogo) ? params->movestogo : 40.0;
        clock_t time = (board->side_to_move == WHITE) ? params->wtime : params->btime;
        clock_t inc = (board->side_to_move == WHITE) ? params->winc : params->binc;

        time = max(0, time - overhead);

        tm->average_time = time / mtg + inc;
        tm->maximal_time = time / sqrt(mtg) + inc;
        tm->average_time = min(tm->average_time, time);
        tm->maximal_time = min(tm->maximal_time, time);
        tm->optimal_time = tm->maximal_time;
    }
    else if (params->movetime)
    {
        tm->mode = Movetime;
        tm->average_time = tm->maximal_time = tm->optimal_time
            = max(1, params->movetime - overhead);
    }
    else
        tm->mode = NoTimeman;

    tm->prev_score = NO_SCORE;
    tm->prev_bestmove = NO_MOVE;
    tm->stability = 0;
    tm->type = NO_BM_TYPE;
}
