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

#ifndef TIMEMAN_H
#define TIMEMAN_H

#include "board.h"
#include "chess_types.h"
#include "core.h"
#include "search_params.h"

// Enum for the type of time management to use
typedef enum {
    TmNone,
    TmMovetime,
    TmTournament,
} TimemanMode;

// Struct for time management
typedef struct {
    Timepoint start;
    TimemanMode mode;
    bool pondering;
    u64 delay_check_nodes;

    Duration average_time;
    Duration maximal_time;
    Duration optimal_time;

    Score previous_score;
    Move previous_bestmove;
    u16 stability;
} Timeman;

// Initializes the time manager based on search parameters
void timeman_init(
    Timeman *restrict timeman,
    const Board *restrict root_board,
    const SearchParams *restrict search_params,
    Timepoint start
);

// Updates the time manager based on the current bestmove and score
void timeman_update(
    Timeman *restrict timeman,
    const Board *restrict root_board,
    Move bestmove,
    Score root_score
);

// Forward declaration required to avoid cyclic include paths.
struct WorkerPool;

// Checks if the time manager thinks we have spent enough time in search
bool timeman_can_stop_search(
    const Timeman *timeman,
    const struct WorkerPool *wpool,
    Timepoint current_tp
);

// Checks if the time manager says we must interrupt the search now
bool timeman_must_stop_search(
    const Timeman *timeman,
    const struct WorkerPool *wpool,
    Timepoint current_tp
);

#endif
