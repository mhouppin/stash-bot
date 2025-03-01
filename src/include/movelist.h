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

#ifndef MOVELIST_H
#define MOVELIST_H

#include "board.h"
#include "chess_types.h"
#include "core.h"
#include "hashkey.h"

// Structure for holding moves along with their score
typedef struct {
    Move move;
    i32 score;
} ExtendedMove;

// Structure for holding a list of moves
typedef struct {
    ExtendedMove moves[MAX_MOVES];
    ExtendedMove *end;
} Movelist;

// Generates all legal moves for the given board and stores them in the given move list
ExtendedMove *extmove_generate_legal(ExtendedMove *restrict movelist, const Board *restrict board);

// Generates all pseudo-legal moves for the given board (only for not in-check positions) and stores
// them in the given move list
ExtendedMove *
    extmove_generate_standard(ExtendedMove *restrict movelist, const Board *restrict board);

// Generates all pseudo-legal moves for the given board (only for in-check positions) and stores
// them in the given move list
ExtendedMove *
    extmove_generate_incheck(ExtendedMove *restrict movelist, const Board *restrict board);

// Generates all pseudo-legal captures/promotions for the given board (only for not in-check
// positions) and stores them in the given move list
ExtendedMove *extmove_generate_noisy(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    bool in_qsearch
);

// Generates all pseudo-legal non-captures/non-promotions for the given board (only for not in-check
// positions) and stores them in the given move list
ExtendedMove *extmove_generate_quiet(ExtendedMove *restrict movelist, const Board *restrict board);

// Places the move with the highest score in the first position of the move list
void extmove_pick_best(ExtendedMove *begin, ExtendedMove *end);

// Generates all legal moves for the given board
INLINED void movelist_generate_legal(Movelist *restrict movelist, const Board *restrict board) {
    movelist->end = extmove_generate_legal(movelist->moves, board);
}

// Generates all pseudo-legal moves for the given board
INLINED void movelist_generate_pseudo(Movelist *restrict movelist, const Board *restrict board) {
    movelist->end = board->stack->checkers ? extmove_generate_incheck(movelist->moves, board)
                                           : extmove_generate_standard(movelist->moves, board);
}

// Returns the size of the move list
INLINED usize movelist_size(const Movelist *movelist) {
    return (usize)(movelist->end - movelist->moves);
}

// Returns the start of the move list
INLINED const ExtendedMove *movelist_begin(const Movelist *movelist) {
    return movelist->moves;
}

// Returns the end of the move list
INLINED const ExtendedMove *movelist_end(const Movelist *movelist) {
    return movelist->end;
}

// Checks if the move list contains the given move
bool movelist_contains(const Movelist *movelist, Move move);

#endif
