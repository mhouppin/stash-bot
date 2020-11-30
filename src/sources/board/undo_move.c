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

void	undo_move(board_t *board, move_t move)
{
	board->side_to_move = not_color(board->side_to_move);

	color_t		us = board->side_to_move;
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);
	piece_t		piece = piece_on(board, to);

	if (type_of_move(move) == PROMOTION)
	{
		remove_piece(board, to);
		piece = create_piece(us, PAWN);
		put_piece(board, piece, to);
	}

	if (type_of_move(move) == CASTLING)
	{
		square_t	rook_from;
		square_t	rook_to;
		undo_castling(board, us, from, &to, &rook_from, &rook_to);
	}
	else
	{
		move_piece(board, to, from);

		if (board->stack->captured_piece)
		{
			square_t	capture_square = to;

			if (type_of_move(move) == EN_PASSANT)
				capture_square -= pawn_direction(us);

			put_piece(board, board->stack->captured_piece, capture_square);
		}
	}

	board->stack = board->stack->prev;
	board->ply -= 1;
}
