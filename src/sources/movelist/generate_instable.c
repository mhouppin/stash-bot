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

#include "movelist.h"

extmove_t	*generate_instable(extmove_t *movelist, const board_t *board)
{
	color_t		us = board->side_to_move;
	bitboard_t	pinned = board->stack->king_blockers[us]
		& board->color_bits[us];
	square_t	king_square = board->piece_list[create_piece(us, KING)][0];
	extmove_t	*current = movelist;

	movelist = board->stack->checkers ? generate_evasions(movelist, board)
		: generate_captures(movelist, board);

	while (current < movelist)
	{
		if ((pinned || move_from_square(current->move) == king_square
			|| type_of_move(current->move) == EN_PASSANT)
			&& !board_legal(board, current->move))
			current->move = (--movelist)->move;
		else
			++current;
	}

	return (movelist);
}
