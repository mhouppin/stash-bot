/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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
#include "movelist.h"

const double   BestmoveTypeScale[BM_TYPE_NB] = {
    0.20,
    0.35,
    0.45,
    0.50,
    0.85,
    0.95,
    1.00,
    1.20,
    1.40,
};

const double    BestmoveStabilityScale[5] = {
    2.50,
    1.20,
    0.90,
    0.80,
    0.75
};

double  score_difference_scale(score_t s)
{
    const score_t   X = 100;
    const double    T = 2.0;

    return (pow(T, clamp(s, -X, X) / (double)X));
}

void    timeman_update(timeman_t *tm, const board_t *board, move_t bestmove,
        score_t score)
{
    // Only update timeman when we need one
    if (tm->mode != Tournament)
        return ;

    // Update bestmove + stability statistics
    if (tm->prev_bestmove != bestmove)
    {
        movelist_t  list;
        bool        is_quiet = !is_capture_or_promotion(board, bestmove);
        bool        gives_check = move_gives_check(board, bestmove);

        tm->prev_bestmove = bestmove;
        tm->stability = 0;

        // Do we only have one legal move ? Don't burn much time on these
        list_all(&list, board);
        if (movelist_size(&list) == 1)
            tm->type = OneLegalMove;

        else if (type_of_move(bestmove) == PROMOTION)
            tm->type = Promotion;

        else if (!is_quiet && see_greater_than(board, bestmove, KNIGHT_MG_SCORE))
            tm->type = SoundCapture;

        else if (gives_check && see_greater_than(board, bestmove, 0))
            tm->type = SoundCheck;

        else if (!is_quiet)
            tm->type = Capture;

        else if (see_greater_than(board, bestmove, 0))
            tm->type = Quiet;

        else if (gives_check)
            tm->type = WeirdCheck;

        else
            tm->type = WeirdQuiet;
    }
    else
        tm->stability = min(tm->stability + 1, 4);

    // Does the move mates, or are we getting mated ? We use very small
    // thinking times for these, because the game result is already known.
    if (abs(score) > MATE_FOUND)
        tm->type = MatingMove;

    // Scale the time usage based on the type of bestmove we have
    double  scale = BestmoveTypeScale[tm->type];

    // Scale the time usage based on how long this bestmove has held
    // through search iterations
    scale *= BestmoveStabilityScale[tm->stability];

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching)
    if (tm->prev_score != NO_SCORE)
        scale *= score_difference_scale(tm->prev_score - score);

    // Update score + optimal time usage
    tm->prev_score = score;
    tm->optimal_time = min(tm->maximal_time, tm->average_time * scale);
}
