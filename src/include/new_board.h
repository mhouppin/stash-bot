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

#ifndef BOARD_H
#define BOARD_H

#include "new_bitboard.h"
#include "new_chess_types.h"
#include "new_core.h"
#include "new_hashkey.h"

// Struct representing the board stack data from past moves
typedef struct _Boardstack
{
    Key board_key;
    Key king_pawn_key;
    Key material_key;
    Bitboard checkers;
    Bitboard king_blockers[COLOR_NB];
    Bitboard pinners[COLOR_NB];
    Bitboard check_squares[PIECETYPE_NB];
    struct _Boardstack *previous;
    CastlingRights castlings;
    u8 rule50;
    u8 repetition;
    u8 plies_since_nullmove;
    Square ep_square;
    Piece captured_piece;
    Score material[COLOR_NB];
} Boardstack;

// Struct representing the board
typedef struct _Board
{
    Piece table[SQUARE_NB];
    Bitboard piecetype_bb[PIECETYPE_NB];
    Bitboard color_bb[COLOR_NB];
    u8 piece_count[PIECE_NB];
    CastlingRights castling_mask[SQUARE_NB];
    Square castling_rook_square[CASTLING_NB];
    Bitboard castling_path[CASTLING_NB];
    Boardstack *stack;
    u16 ply;
    Color side_to_move;
    Scorepair psq_scorepair;
    bool chess960;
} Board;

// Initializes cycle detection tables
void cyclic_init(void);

// Initializes the board from the given FEN string
void
#endif
