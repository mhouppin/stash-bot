/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   set_castling.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 08:11:21 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 10:57:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include "board.h"

void	set_castling(board_t *board, color_t color, square_t rook_square)
{
	square_t	king_square = board->piece_list[create_piece(color, KING)][0];
	int			castling = (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING)
		& (king_square < rook_square ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

	board->stack->castlings |= castling;
	board->castling_mask[king_square] |= castling;
	board->castling_mask[rook_square] |= castling;
	board->castling_rook_square[castling] = rook_square;

	square_t	king_after = relative_square(
		castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1, color);
	square_t	rook_after = relative_square(
		castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1, color);

	board->castling_path[castling] = (squares_between(rook_square, rook_after)
		| squares_between(king_square, king_after) | square_bit(rook_after)
		| square_bit(king_after)) & ~(square_bit(king_square)
		| square_bit(rook_square));
}
