/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#include "board.h"

bitboard_t  attackers_list(const board_t *board, square_t s,
            bitboard_t occupied)
{
    return ((pawn_moves(s, BLACK) & piece_bb(board, WHITE, PAWN))
        | (pawn_moves(s, WHITE) & piece_bb(board, BLACK, PAWN))
        | (knight_moves(s) & piecetype_bb(board, KNIGHT))
        | (rook_moves_bb(s, occupied) & piecetypes_bb(board, ROOK, QUEEN))
        | (bishop_moves_bb(s, occupied) & piecetypes_bb(board, BISHOP, QUEEN))
        | (king_moves(s) & piecetype_bb(board, KING)));
}
