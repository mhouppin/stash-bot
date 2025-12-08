/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "search.h"
#include "worker.h"

// Enum for the various stages of the move picker
typedef enum {
    PICK_TT,
    GEN_NOISY,
    PICK_GOOD_NOISY,
    PICK_KILLER,
    PICK_COUNTER,
    GEN_QUIETS,
    PICK_QUIETS,
    PICK_BAD_NOISY,

    CHECK_PICK_TT,
    CHECK_GEN_ALL,
    CHECK_PICK_ALL
} MovepickerStage;

// Struct for the move picker
typedef struct {
    usize move_count;
    usize current_idx;
    usize bad_captures_idx;
    bool in_qsearch;
    MovepickerStage stage;
    Move tt_move;
    Move killer;
    Move counter;
    const Board *board;
    const Worker *worker;
    PieceHistory *piece_history[2];
    Move move_list[MAX_MOVES];
    i32 score_list[MAX_MOVES];
} Movepicker;

// Initializes the move picker
void movepicker_init(
    Movepicker *mp,
    bool in_qsearch,
    const Board *board,
    const Worker *worker,
    Move tt_move,
    Searchstack *ss
);

// Returns the next best move according to the move picker, with the option to skip quiet moves
Move movepicker_next_move(Movepicker *mp, bool skip_quiets, Score see_threshold);

#endif
