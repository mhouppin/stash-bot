/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   set_boardstack.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 08:19:24 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 07:52:00 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

void	set_boardstack(board_t *board, boardstack_t *stack)
{
	stack->board_key = 0;
	stack->checkers = attackers_to(board, board->piece_list[
		create_piece(board->side_to_move, KING)][0])
		& board->color_bits[opposite_color(board->side_to_move)];

	set_check(board, stack);

	for (bitboard_t b = board->piecetype_bits[ALL_PIECES]; b; )
	{
		square_t	square = pop_first_square(&b);
		piece_t		piece = piece_on(board, square);

		stack->board_key ^= ZobristPsq[piece][square];
	}

	if (stack->en_passant_square != SQ_NONE)
		stack->board_key ^= ZobristEnPassant[
			file_of_square(stack->en_passant_square)];

	if (board->side_to_move == BLACK)
		stack->board_key ^= ZobristBlackToMove;

	stack->board_key ^= ZobristCastling[stack->castlings];
}
