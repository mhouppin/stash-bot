/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   do_null_move.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/11 11:22:41 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/11 12:23:30 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <string.h>
#include "board.h"
#include "info.h"
#include "tt.h"

void	do_null_move(board_t *board, boardstack_t *stack)
{
	g_nodes += 1;

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
