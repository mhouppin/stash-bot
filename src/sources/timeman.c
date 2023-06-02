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

#include "timeman.h"
#include "movelist.h"
#include "types.h"
#include "uci.h"
#include "worker.h"
#include <math.h>

// Scaling table based on the move type
const double BestmoveTypeScale[BM_TYPE_NB] = {
    0.20, // One legal move
    0.45, // Promoting a piece
    0.50, // Capture with a very high SEE
    0.85, // Check not throwing away material
    0.95, // Capture
    1.00, // Quiet move not throwing away material
    1.20, // Check losing material
    1.40, // Quiet losing material
};

INLINED clock_t timemin(clock_t left, clock_t right) { return (left < right) ? left : right; }

// Unused for now, commented to avoid compilation issues with the function being
// declared static. Uncomment if needed for time management code updates.
// INLINED clock_t timemax(clock_t left, clock_t right) { return (left > right) ? left : right; }

// Scaling table based on the number of consecutive iterations the bestmove held
const double BestmoveStabilityScale[5] = {2.50, 1.20, 0.90, 0.80, 0.75};

void timeman_init(const Board *board, Timeman *tm, SearchParams *params, clock_t start)
{
    clock_t overhead = UciOptionFields.moveOverhead;

    tm->start = start;
    tm->pondering = false;
    tm->checkFrequency = 1000;

    if (params->nodes) tm->checkFrequency = (int)fmin(1000.0, sqrt(params->nodes) + 0.5);

    if (params->wtime || params->btime)
    {
        tm->mode = Tournament;

        double mtg = (params->movestogo) ? params->movestogo : 40.0;
        clock_t time = (board->sideToMove == WHITE) ? params->wtime : params->btime;
        clock_t inc = (board->sideToMove == WHITE) ? params->winc : params->binc;

        // Don't let time underflow here.
        time -= timemin(time, overhead);

        tm->averageTime = time / mtg + inc;
        tm->maximalTime = time / pow(mtg, 0.4) + inc;

        // Allow for more time usage when we're pondering, since we're not using
        // our clock as long as the opponent thinks.
        if (params->ponder)
        {
            tm->pondering = true;
            tm->averageTime += tm->averageTime / 4;
        }

        tm->averageTime = timemin(tm->averageTime, time);
        tm->maximalTime = timemin(tm->maximalTime, time);
        tm->optimalTime = tm->maximalTime;

        // Log the maximal time in debug mode.
        debug_printf("info maximal_time %" FMT_INFO "\n", (info_t)tm->maximalTime);
    }
    else if (params->movetime)
    {
        tm->mode = Movetime;
        tm->averageTime = tm->maximalTime = tm->optimalTime =
            (params->movetime <= overhead) ? 1 : (params->movetime - overhead);

        // Log the maximal time in debug mode.
        debug_printf("info maximal_time %" FMT_INFO "\n", (info_t)tm->maximalTime);
    }
    else
        tm->mode = NoTimeman;

    tm->prevScore = NO_SCORE;
    tm->prevBestmove = NO_MOVE;
    tm->stability = 0;
    tm->type = NO_BM_TYPE;
}

double score_difference_scale(score_t s)
{
    const score_t X = 100;
    const double T = 2.0;

    // Clamp score to the range [-100, 100], and convert it to a time scale in
    // the range [0.5, 2.0].
    // Examples:
    // -100 -> 2.000x time
    //  -50 -> 1.414x time
    //    0 -> 1.000x time
    //  +50 -> 0.707x time
    // +100 -> 0.500x time
    return pow(T, iclamp(s, -X, X) / (double)X);
}

void timeman_update(Timeman *tm, const Board *board, move_t bestmove, score_t score)
{
    // Only update the timeman when we need one.
    if (tm->mode != Tournament) return;

    // Update bestmove + stability statistics.
    if (tm->prevBestmove != bestmove)
    {
        Movelist list;
        bool isQuiet = !is_capture_or_promotion(board, bestmove);
        bool givesCheck = move_gives_check(board, bestmove);

        tm->prevBestmove = bestmove;
        tm->stability = 0;

        // Do we only have one legal move ? Don't burn much time on these.
        list_all(&list, board);
        if (movelist_size(&list) == 1)
            tm->type = OneLegalMove;

        else if (move_type(bestmove) == PROMOTION)
            tm->type = Promotion;

        else if (!isQuiet && see_greater_than(board, bestmove, KNIGHT_MG_SCORE))
            tm->type = SoundCapture;

        else if (givesCheck && see_greater_than(board, bestmove, 0))
            tm->type = SoundCheck;

        else if (!isQuiet)
            tm->type = Capture;

        else if (see_greater_than(board, bestmove, 0))
            tm->type = Quiet;

        else if (givesCheck)
            tm->type = WeirdCheck;

        else
            tm->type = WeirdQuiet;
    }
    else
        tm->stability = imin(tm->stability + 1, 4);

    // Scale the time usage based on the type of bestmove we have.
    double scale = BestmoveTypeScale[tm->type];

    // Scale the time usage based on how long this bestmove has held
    // through search iterations.
    scale *= BestmoveStabilityScale[tm->stability];

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching).
    if (tm->prevScore != NO_SCORE) scale *= score_difference_scale(tm->prevScore - score);

    // Update score + optimal time usage.
    tm->prevScore = score;
    tm->optimalTime = timemin(tm->maximalTime, tm->averageTime * scale);

    // Log the optimal time in debug mode.
    debug_printf("info optimal_time %" FMT_INFO "\n", (info_t)tm->optimalTime);
}

void check_time(void)
{
    if (--SearchWorkerPool.checks > 0) return;

    // Reset the verification counter.
    SearchWorkerPool.checks = SearchTimeman.checkFrequency;

    // If we are in infinite mode, or the stop has already been set,
    // we can safely return.
    if (UciSearchParams.infinite || SearchWorkerPool.stop) return;

    // Check if we went over the requested node count.
    if (wpool_get_total_nodes(&SearchWorkerPool) >= UciSearchParams.nodes) goto __set_stop;

    // Check if we went over maximal time usage.
    if (timeman_must_stop_search(&SearchTimeman, chess_clock())) goto __set_stop;

    return;

__set_stop:
    SearchWorkerPool.stop = true;
}
