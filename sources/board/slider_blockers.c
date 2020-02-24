/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   slider_blockers.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 11:08:07 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 12:00:09 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

bitboard_t		slider_blockers(const board_t *board, bitboard_t sliders,
				square_t square, bitboard_t *pinners)
{
	bitboard_t	blockers = *pinners = 0;
	bitboard_t	snipers = (
		(PseudoMoves[ROOK][square] & board_pieces(board, QUEEN, ROOK))
		| (PseudoMoves[BISHOP][square] & board_pieces(board, QUEEN, BISHOP)))
		& sliders;
	bitboard_t	occupied = board->piecetype_bits[ALL_PIECES] ^ snipers;

	while (snipers)
	{
		square_t	sniper_square = pop_first_square(&snipers);
		bitboard_t	between = squares_between(square, sniper_square) & occupied;

		if (between && !more_than_one(between))
		{
			blockers |= between;
			if (between & board->color_bits[color_of_piece(piece_on(board,
					square))])
				*pinners |= square_bit(sniper_square);
		}
	}

	return (blockers);
}
