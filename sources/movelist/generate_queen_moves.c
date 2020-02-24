/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_queen_moves.c                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 14:39:14 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 14:41:35 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

extmove_t	*generate_queen_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target)
{
	const square_t	*piecelist = board->piece_list[create_piece(us, QUEEN)];

	for (square_t from = *piecelist; from != SQ_NONE; from = *++piecelist)
	{
		bitboard_t	b = queen_moves(board, from) & target;

		while (b)
			(movelist++)->move = create_move(from, pop_first_square(&b));
	}

	return (movelist);
}
