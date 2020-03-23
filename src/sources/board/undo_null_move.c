/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   undo_null_move.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/11 11:27:04 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/11 11:31:21 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

void	undo_null_move(board_t *board)
{
	board->stack = board->stack->prev;
	board->side_to_move = opposite_color(board->side_to_move);
}
