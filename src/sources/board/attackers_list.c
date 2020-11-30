/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "board.h"

bitboard_t		attackers_list(const board_t *board, square_t s,
				bitboard_t occupied)
{
	return ((pawn_moves(s, BLACK) & board->piecetype_bits[PAWN]
		& board->color_bits[WHITE])
		| (pawn_moves(s, WHITE) & board->piecetype_bits[PAWN]
		& board->color_bits[BLACK])
		| (knight_moves(s) & board->piecetype_bits[KNIGHT])
		| (rook_move_bits(s, occupied) & piecetypes_bb(board, ROOK, QUEEN))
		| (bishop_move_bits(s, occupied) & piecetypes_bb(board, BISHOP, QUEEN))
		| (king_moves(s) & board->piecetype_bits[KING]));
}
