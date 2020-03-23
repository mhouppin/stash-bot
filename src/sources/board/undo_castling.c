/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   undo_castling.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 18:40:45 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 07:22:16 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

void	undo_castling(board_t *board, color_t us, square_t king_from,
		square_t *king_to, square_t *rook_from, square_t *rook_to)
{
	bool	kingside = *king_to > king_from;

	*rook_from = *king_to;
	*rook_to = relative_square(kingside ? SQ_F1 : SQ_D1, us);
	*king_to = relative_square(kingside ? SQ_G1 : SQ_C1, us);

	remove_piece(board, *king_to);
	remove_piece(board, *rook_to);
	board->table[*king_to] = board->table[*rook_to] = NO_PIECE;
	put_piece(board, create_piece(us, KING), king_from);
	put_piece(board, create_piece(us, ROOK), *rook_from);
}
