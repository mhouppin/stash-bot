/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   undo_move.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 14:02:24 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/22 14:08:06 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

void	undo_move(board_t *board, move_t move)
{
	board->side_to_move = opposite_color(board->side_to_move);

	color_t		us = board->side_to_move;
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);
	piece_t		piece = piece_on(board, to);

	if (type_of_move(move) == PROMOTION)
	{
		remove_piece(board, to);
		piece = create_piece(us, PAWN);
		put_piece(board, piece, to);
	}

	if (type_of_move(move) == CASTLING)
	{
		square_t	rook_from;
		square_t	rook_to;
		undo_castling(board, us, from, &to, &rook_from, &rook_to);
	}
	else
	{
		move_piece(board, to, from);

		if (board->stack->captured_piece)
		{
			square_t	capture_square = to;

			if (type_of_move(move) == EN_PASSANT)
				capture_square -= pawn_direction(us);

			put_piece(board, board->stack->captured_piece, capture_square);
		}
	}

	board->stack = board->stack->prev;
	board->ply -= 1;
}
