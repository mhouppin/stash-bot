/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   set_check.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 10:57:14 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/21 11:07:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

void	set_check(board_t *board, boardstack_t *stack)
{
	stack->king_blockers[WHITE] = slider_blockers(board,
		board->color_bits[BLACK], board->piece_list[WHITE_KING][0],
		&stack->pinners[BLACK]);
	stack->king_blockers[BLACK] = slider_blockers(board,
		board->color_bits[WHITE], board->piece_list[BLACK_KING][0],
		&stack->pinners[WHITE]);

	square_t	king_square = board->piece_list[
		create_piece(opposite_color(board->side_to_move), KING)][0];

	stack->check_squares[PAWN] = pawn_moves(king_square,
		opposite_color(board->side_to_move));

	stack->check_squares[KNIGHT] = knight_moves(king_square);
	stack->check_squares[BISHOP] = bishop_moves(board, king_square);
	stack->check_squares[ROOK] = rook_moves(board, king_square);
	stack->check_squares[QUEEN] = stack->check_squares[BISHOP]
		| stack->check_squares[ROOK];
	stack->check_squares[KING] = 0;
}
