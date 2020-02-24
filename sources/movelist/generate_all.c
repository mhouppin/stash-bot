/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_all.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 16:17:34 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:18:14 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

extmove_t	*generate_all(extmove_t *movelist, const board_t *board)
{
	color_t		us = board->side_to_move;

	bitboard_t	pinned = board->stack->king_blockers[us]
		& board->color_bits[us];

	square_t	king_square = board->piece_list[create_piece(us, KING)][0];

	extmove_t	*current = movelist;

	movelist = board->stack->checkers ? generate_evasions(movelist, board)
		: generate_classic(movelist, board);

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
