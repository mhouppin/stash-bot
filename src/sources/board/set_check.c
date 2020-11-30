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

void	set_check(board_t *board, boardstack_t *stack)
{
	stack->king_blockers[WHITE] = slider_blockers(board,
		board->color_bits[BLACK], board_king_square(board, WHITE),
		&stack->pinners[BLACK]);
	stack->king_blockers[BLACK] = slider_blockers(board,
		board->color_bits[WHITE], board_king_square(board, BLACK),
		&stack->pinners[WHITE]);

	square_t	king_square = board_king_square(board, not_color(board->side_to_move));

	stack->check_squares[PAWN] = pawn_moves(king_square,
		not_color(board->side_to_move));

	stack->check_squares[KNIGHT] = knight_moves(king_square);
	stack->check_squares[BISHOP] = bishop_moves(board, king_square);
	stack->check_squares[ROOK] = rook_moves(board, king_square);
	stack->check_squares[QUEEN] = stack->check_squares[BISHOP]
		| stack->check_squares[ROOK];
	stack->check_squares[KING] = 0;
}
