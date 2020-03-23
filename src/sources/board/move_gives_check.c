/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   move_gives_check.c                               .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 20:40:02 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:00:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

bool	move_gives_check(const board_t *board, move_t move)
{
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);

	if (board->stack->check_squares[type_of_piece(piece_on(board, from))]
		& square_bit(to))
		return (true);

	square_t	their_king = board->piece_list[create_piece(
		opposite_color(board->side_to_move), KING)][0];

	if ((board->stack->king_blockers[opposite_color(board->side_to_move)]
		& square_bit(from)) && !aligned(from, to, their_king))
		return (true);

	switch (type_of_move(move))
	{
		case NORMAL_MOVE:
			return (false);

		case PROMOTION:
			return (piece_moves(promotion_type(move), to,
				board->piecetype_bits[ALL_PIECES] ^ square_bit(from))
				& square_bit(their_king));

		case EN_PASSANT:
			(void)0;
			square_t	capture_square = create_square(file_of_square(to),
				rank_of_square(from));

			bitboard_t	occupied = (board->piecetype_bits[ALL_PIECES]
				^ square_bit(from) ^ square_bit(capture_square))
				| square_bit(to);

			return ((rook_move_bits(their_king, occupied)
				& board_colored_pieces(board, board->side_to_move, QUEEN, ROOK))
				| (bishop_move_bits(their_king, occupied)
				& board_colored_pieces(board, board->side_to_move, QUEEN,
				BISHOP)));

		case CASTLING:
			(void)0;
			square_t	king_from = from;
			square_t	rook_from = to;
			square_t	king_to = relative_square(
				rook_from > king_from ? SQ_G1 : SQ_C1, board->side_to_move);
			square_t	rook_to = relative_square(
				rook_from > king_from ? SQ_F1 : SQ_D1, board->side_to_move);

			return ((PseudoMoves[ROOK][rook_to] & square_bit(their_king))
				&& (rook_move_bits(rook_to, (board->piecetype_bits[ALL_PIECES]
				^ square_bit(king_from) ^ square_bit(rook_from))
				| square_bit(king_to) | square_bit(rook_to))
				& square_bit(their_king)));

		default:
			__builtin_unreachable();
			return (false);
	}
}
