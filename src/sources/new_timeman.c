/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include "new_timeman.h"

#include <math.h>

#include "new_movelist.h"

// Scaling table based on the number of consecutive iterations the bestmove held
const f64 BestmoveStabilityScale[5] = {2.50, 1.20, 0.90, 0.80, 0.75};

void timeman_init(
    Timeman *restrict timeman,
    const Board *restrict root_board,
    const SearchParams *restrict search_params,
    Timepoint start
) {
    const Duration overhead = search_params->move_overhead;

    timeman->start = start;
    timeman->pondering = false;
    timeman->delay_check_nodes = 1000;

    if (search_params->nodes != 0) {
        timeman->delay_check_nodes = (u64)fmin(1000.0, sqrt(search_params->nodes) + 0.5);
    }

    if (search_params->tc_is_set) {
        const u16 mtg =
            (search_params->movestogo != 0) ? u16_min(search_params->movestogo, 100) : 40;
        Duration time =
            (root_board->side_to_move == WHITE) ? search_params->wtime : search_params->btime;
        Duration inc =
            (root_board->side_to_move == WHITE) ? search_params->winc : search_params->binc;

        timeman->mode = TmTournament;

        // Don't let time go to or under zero here. This also fixes a problem with some GUIs issuing
        // a negative remaining time.
        time = duration_max(1, time - overhead);
        inc = duration_max(0, inc);

        timeman->average_time = time / (i64)mtg + inc;
        timeman->maximal_time = (Duration)(time / pow(mtg, 0.4)) + inc;

        // Allow for more time usage when we're pondering, since we're not using
        // our clock as long as the opponent thinks.
        if (search_params->ponder) {
            timeman->pondering = true;
            timeman->average_time += timeman->average_time / 4;
        }

        // Don't allow the search to use more time than the remaining time, even with the added
        // increment.
        timeman->average_time = duration_min(time - 1, timeman->average_time);
        timeman->maximal_time = duration_min(time - 1, timeman->maximal_time);
        timeman->optimal_time = timeman->maximal_time;
        // info_debug()
    } else if (search_params->movetime != 0) {
        timeman->mode = TmMovetime;
        timeman->maximal_time =
            duration_max(1, search_params->movetime - search_params->move_overhead);
        timeman->average_time = timeman->maximal_time;
        timeman->optimal_time = timeman->maximal_time;
        // info_debug()
    } else {
        timeman->mode = TmNone;
    }

    timeman->previous_score = NO_SCORE;
    timeman->previous_bestmove = NO_MOVE;
    timeman->stability = 0;
}

f64 timeman_scale_score_diff(i32 score_progression) {
    const i32 k = 100;
    const f64 x = 2.0;

    // Clamp the score progression to the range [-100, 100], and convert it to a time scale in the
    // range [0.5, 2.0]. This is done so that we allot more time when the score starts falling, and
    // less time when it raises.
    // Examples:
    // -100 -> 2.000x time
    //  -50 -> 1.414x time
    //    0 -> 1.000x time
    //  +50 -> 0.707x time
    // +100 -> 0.500x time
    return pow(x, i32_clamp(-score_progression, -k, k) / (f64)k);
}

void timeman_update(
    Timeman *restrict timeman,
    const Board *restrict root_board,
    Move bestmove,
    Score root_score
) {
    if (timeman->mode != TmTournament) {
        return;
    }

    Movelist movelist;
    f64 scale = 1.0;

    movelist_generate_legal(&movelist, root_board);

    // Cut down time usage on positions with a single legal move.
    if (movelist_size(&movelist) == 1) {
        scale = 0.2;
    }

    // Update bestmove + stability statistics.
    if (timeman->previous_bestmove != bestmove) {
        timeman->previous_bestmove = bestmove;
        timeman->stability = 0;
    } else {
        timeman->stability = u16_min(timeman->stability + 1, 4);
    }

    // Scale the time usage based on how long this bestmove has held
    // through search iterations.
    scale *= BestmoveStabilityScale[timeman->stability];

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching).
    if (timeman->previous_score != NO_SCORE) {
        scale *= timeman_scale_score_diff((i32)root_score - (i32)timeman->previous_score);
    }

    // Update score + optimal time usage.
    timeman->previous_score = root_score;
    timeman->optimal_time = duration_min(timeman->maximal_time, timeman->average_time * scale);
    // info_debug()
}

bool timeman_can_stop_search(const Timeman *timeman, Timepoint current_tp) {
    if (timeman->pondering && /* wpool_is_pondering(&SearchWorkerPool) */ true) {
        return false;
    }

    return timeman->mode != TmNone
        && timepoint_diff(timeman->start, current_tp) >= timeman->optimal_time;
}

bool timeman_must_stop_search(const Timeman *timeman, Timepoint current_tp) {
    if (timeman->pondering && /* wpool_is_pondering(&SearchWorkerPool) */ true) {
        return false;
    }

    return timeman->mode != TmNone
        && timepoint_diff(timeman->start, current_tp) >= timeman->maximal_time;
}
