/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_bishop_moves.c                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 13:52:11 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 10:32:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

extmove_t	*generate_bishop_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target)
{
	const square_t	*piecelist = board->piece_list[create_piece(us, BISHOP)];

	for (square_t from = *piecelist; from != SQ_NONE; from = *++piecelist)
	{
		bitboard_t	b = bishop_moves(board, from) & target;

		while (b)
			(movelist++)->move = create_move(from, pop_first_square(&b));
	}

	return (movelist);
}
