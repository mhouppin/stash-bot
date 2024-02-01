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

clock_t chess_clock(void)
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
};

INLINED clock_t timemin(clock_t left, clock_t right) { return (left < right) ? left : right; }

// Unused for now, commented to avoid compilation issues with the function being
// declared static. Uncomment if needed for time management code updates.
// INLINED clock_t timemax(clock_t left, clock_t right) { return (left > right) ? left : right; }

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
}

double score_difference_scale(score_t s)
{
    const score_t x = 112 /* TimemanScoreRange */;

    return pow(2.028 /* TimemanScoreFactor */, iclamp(s, -x, x) / (double)x);
}

double node_repartition_scale(const Worker *worker)
{
    const uint64_t total_nodes = get_worker_nodes(worker);
    const uint64_t best_nodes = worker->rootMoves->nodes;
    const double best_rate = (double)best_nodes / (double)total_nodes;

    return (1.283 /* TimemanNodeBase */ - best_rate) * 1.628 /* TimemanNodeFactor */;
}

double bestmove_stability_scale(int stability)
{
    return 2.639 /* TimemanStabFactor */ / pow(stability + 1, 0.823 /* TimemanStabExponent */);
}

void timeman_update(Timeman *tm, const Board *board, move_t bestmove, score_t score)
{
    // Only update the timeman when we need one.
    if (tm->mode != Tournament) return;

    // Update bestmove + stability statistics.
    if (tm->prevBestmove != bestmove)
    {
        tm->prevBestmove = bestmove;
        tm->stability = 0;
    }
    else
        tm->stability = imin(tm->stability + 1, 3 /* TimemanStabIters */);

    // Scale the time usage based on the root moves' node repartition.
    double scale = node_repartition_scale(get_worker(board));

    // Scale the time usage based on how long this bestmove has held
    // through search iterations.
    scale *= bestmove_stability_scale(tm->stability);

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
    if (UciSearchParams.infinite || wpool_is_stopped(&SearchWorkerPool)) return;

    // Check if we went over the requested node count or the maximal time usage.
    if (wpool_get_total_nodes(&SearchWorkerPool) >= UciSearchParams.nodes
        || timeman_must_stop_search(&SearchTimeman, chess_clock()))
        wpool_stop(&SearchWorkerPool);
}
