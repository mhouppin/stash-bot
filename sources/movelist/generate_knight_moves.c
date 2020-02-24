/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_knight_moves.c                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 14:22:27 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 14:26:47 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

extmove_t	*generate_knight_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target)
{
	const square_t	*piecelist = board->piece_list[create_piece(us, KNIGHT)];

	for (square_t from = *piecelist; from != SQ_NONE; from = *++piecelist)
	{
		bitboard_t	b = knight_moves(from) & target;

		while (b)
			(movelist++)->move = create_move(from, pop_first_square(&b));
	}

	return (movelist);
}
