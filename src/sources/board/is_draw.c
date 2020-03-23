/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   is_draw.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 17:32:49 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/22 17:38:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"
#include "movelist.h"

bool	is_draw(const board_t *board, int ply)
{
	if (board->stack->rule50 > 99)
	{
		if (!board->stack->checkers)
			return (true);

		movelist_t	movelist;

		list_all(&movelist, board);

		if (movelist_size(&movelist) != 0)
			return (true);
	}

	if (board->stack->repetition && board->stack->repetition < ply)
		return (true);

	return (false);
}
