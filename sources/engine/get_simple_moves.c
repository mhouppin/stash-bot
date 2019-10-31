/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   get_simple_moves.c                               .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 01:21:23 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 05:59:09 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>

movelist_t	*get_simple_moves(board_t *board)
{
	movelist_t	*mlist;

	mlist = movelist_init();
	if (mlist == NULL)
		return (NULL);

	for (int8_t i = SQ_A1; i <= SQ_H8; i++)
	{
		int8_t	piece = board->table[i] ^ (board->player << 3);

		switch (piece)
		{
			case WHITE_PAWN:
				get_pawn_moves(mlist, i, board);
				break ;

			case WHITE_KNIGHT:
				get_knight_moves(mlist, i, board);
				break ;

			case WHITE_BISHOP:
				get_bishop_moves(mlist, i, board);
				break ;

			case WHITE_ROOK:
				get_rook_moves(mlist, i, board);
				break ;

			case WHITE_QUEEN:
				get_bishop_moves(mlist, i, board);
				get_rook_moves(mlist, i, board);
				break ;

			case WHITE_KING:
				get_king_moves(mlist, i, board);
				break ;
		}
	}

	board_t	copy;

	size_t	m = 0;
	while (m < mlist->size)
	{
		copy = *board;
		do_move(&copy, mlist->moves[m]);
		if (is_checked(&copy))
			pop_move(mlist, m);
		else
			m++;
	}

	return (mlist);
}
