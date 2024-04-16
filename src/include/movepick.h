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

#ifndef MOVEPICK_H
#define MOVEPICK_H

#include "movelist.h"
#include "search.h"
#include "worker.h"

// Enum for various stages of the move picker
typedef enum mp_stage_e
{
    PICK_TT,
    GEN_INSTABLE,
    PICK_GOOD_INSTABLE,
    PICK_REFUTATION,
    GEN_QUIET,
    PICK_QUIET,
    PICK_BAD_INSTABLE,

    CHECK_PICK_TT,
    CHECK_GEN_ALL,
    CHECK_PICK_ALL
} mp_stage_t;

// Struct for the move picker
typedef struct _Movepicker
{
    Movelist list;
    ExtendedMove *cur, *end, *badCaptures;
    ExtendedMove refutations[3];
    bool inQsearch;
    mp_stage_t stage;
    move_t ttMove;
    const Board *board;
    const Worker *worker;
    piece_history_t *pieceHistory[2];
} Movepicker;

// Initializes the move picker.
void movepicker_init(Movepicker *mp, bool inQsearch, const Board *board, const Worker *worker,
    move_t ttMove, Searchstack *ss);

// Returns the next move in the move picker, with the option to skip quiet moves.
move_t movepicker_next_move(Movepicker *mp, bool skipQuiets, int see_threshold);

INLINED bool movepicker_is_refutation(const Movepicker *mp, move_t move)
{
    return move == mp->refutations[0].move || move == mp->refutations[1].move
        || move == mp->refutations[2].move;
}

#endif
