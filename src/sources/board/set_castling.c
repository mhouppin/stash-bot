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

#include <stdio.h>
#include "board.h"

void	set_castling(board_t *board, color_t color, square_t rook_square)
{
	square_t	king_square = board_king_square(board, color);
	int			castling = (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING)
		& (king_square < rook_square ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

	board->stack->castlings |= castling;
	board->castling_mask[king_square] |= castling;
	board->castling_mask[rook_square] |= castling;
	board->castling_rook_square[castling] = rook_square;

	square_t	king_after = relative_square(
		castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1, color);
	square_t	rook_after = relative_square(
		castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1, color);

	board->castling_path[castling] = (squares_between(rook_square, rook_after)
		| squares_between(king_square, king_after) | square_bit(rook_after)
		| square_bit(king_after)) & ~(square_bit(king_square)
		| square_bit(rook_square));
}
