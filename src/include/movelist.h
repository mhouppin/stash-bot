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

#ifndef MOVELIST_H
#define MOVELIST_H

#include "board.h"
#include "types.h"
#include <stddef.h>

// Structure for holding moves along with their score
typedef struct _ExtendedMove
{
    move_t move;
    score_t score;
} ExtendedMove;

// Structure for holding a list of moves
typedef struct _MoveList
{
    ExtendedMove moves[256];
    ExtendedMove *last;
} Movelist;

// Global list for the "go searchmoves" option
extern Movelist UciSearchMoves;

// Generates all legal moves for the given board and stores them in the given movelist.
ExtendedMove *generate_all(ExtendedMove *movelist, const Board *board);

// Generates all pseudo-legal moves for the given board (only for not in-check positions) and stores
// them in the given movelist.
ExtendedMove *generate_classic(ExtendedMove *movelist, const Board *board);

// Generates all pseudo-legal moves for the given board (only for in-check positions) and stores
// them in the given movelist.
ExtendedMove *generate_evasions(ExtendedMove *movelist, const Board *board);

// Generates all pseudo-legal captures/queen promotions for the given board and stores them in the
// given movelist.
ExtendedMove *generate_captures(ExtendedMove *movelist, const Board *board, bool inQsearch);

// Generates all pseudo-legal non-captures/non-queen promotions for the given board and stores them
// in the given movelist.
ExtendedMove *generate_quiet(ExtendedMove *movelist, const Board *board);

// Places the move with the highest score in the first position of the movelist.
void place_top_move(ExtendedMove *begin, ExtendedMove *end);

// Generates all legal moves for the given board.
INLINED void list_all(Movelist *movelist, const Board *board)
{
    movelist->last = generate_all(movelist->moves, board);
}

// Generates all pseudo-legal moves for the given board.
INLINED void list_pseudo(Movelist *movelist, const Board *board)
{
    movelist->last = board->stack->checkers ? generate_evasions(movelist->moves, board)
                                            : generate_classic(movelist->moves, board);
}

// Returns the size of the movelist.
INLINED size_t movelist_size(const Movelist *movelist)
{
    return (movelist->last - movelist->moves);
}

// Returns the start of the movelist.
INLINED const ExtendedMove *movelist_begin(const Movelist *movelist) { return (movelist->moves); }

// Returns the end of the movelist.
INLINED const ExtendedMove *movelist_end(const Movelist *movelist) { return (movelist->last); }

// Checks if the movelist contains the given move.
INLINED bool movelist_has_move(const Movelist *movelist, move_t move)
{
    for (const ExtendedMove *extmove = movelist_begin(movelist); extmove < movelist_end(movelist);
         ++extmove)
        if (extmove->move == move) return (true);

    return (false);
}

#endif // MOVELIST_H
