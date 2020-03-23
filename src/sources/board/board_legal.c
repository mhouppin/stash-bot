/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   board_legal.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 15:19:46 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:24:31 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

bool	board_legal(const board_t *board, move_t move)
{
	color_t		us = board->side_to_move;
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);

	if (type_of_move(move) == EN_PASSANT)
	{
		square_t	king_square = board->piece_list[create_piece(us, KING)][0];
		square_t	capture_square = to - pawn_direction(us);
		bitboard_t	occupied = ((board->color_bits[WHITE]
			| board->color_bits[BLACK]) ^ square_bit(from)
			^ square_bit(capture_square)) | square_bit(to);

		return (!(rook_move_bits(king_square, occupied)
			& board_colored_pieces(board, opposite_color(us), QUEEN, ROOK))
			&& !(bishop_move_bits(king_square, occupied)
			& board_colored_pieces(board, opposite_color(us), QUEEN, BISHOP)));
	}

	if (type_of_move(move) == CASTLING)
	{
		to = relative_square((to > from ? SQ_G1 : SQ_C1), us);

		direction_t side = (to > from ? WEST : EAST);

		for (square_t sq = to; sq != from; sq += side)
			if (attackers_to(board, sq) & board->color_bits[opposite_color(us)])
				return (false);

		return (!board->chess960
			|| !(rook_move_bits(to, board->piecetype_bits[ALL_PIECES]
			^ square_bit(move_to_square(move))) & board_colored_pieces(board,
			opposite_color(us), ROOK, QUEEN)));
	}

	if (type_of_piece(piece_on(board, from)) == KING)
		return (!(attackers_to(board, to)
			& board->color_bits[opposite_color(us)]));

	return (!(board->stack->king_blockers[us] & square_bit(from))
		|| aligned(from, to, board->piece_list[create_piece(us, KING)][0]));
}
