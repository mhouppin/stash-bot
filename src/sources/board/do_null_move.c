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

#include <string.h>
#include "board.h"
#include "info.h"
#include "lazy_smp.h"
#include "tt.h"

void	do_null_move(board_t *board, boardstack_t *stack)
{
	get_worker(board)->nodes += 1;

	memcpy(stack, board->stack, sizeof(boardstack_t));
	stack->prev = board->stack;
	board->stack = stack;

	if (stack->en_passant_square != SQ_NONE)
	{
		stack->board_key ^= ZobristEnPassant[
			file_of_square(stack->en_passant_square)];
		stack->en_passant_square = SQ_NONE;
	}

	stack->board_key ^= ZobristBlackToMove;
	prefetch(tt_entry_at(stack->board_key));

	++stack->rule50;
	stack->plies_from_null_move = 0;

	board->side_to_move = opposite_color(board->side_to_move);

	set_check(board, stack);

	stack->repetition = 0;
}
