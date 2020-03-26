/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   see_greater_than.c                               .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:13:19 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/11 11:26:57 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

bool	see_greater_than(const board_t *board, move_t m, score_t threshold)
{
	if (type_of_move(m) != NORMAL_MOVE)
		return (threshold <= 0);

	square_t	from = move_from_square(m);
	square_t	to = move_to_square(m);

	int			next_score = PieceScores[MIDGAME][piece_on(board, to)] - threshold;

	if (next_score < 0)
		return (false);

	next_score = PieceScores[MIDGAME][piece_on(board, from)] - next_score;

	if (next_score <= 0)
		return (true);

	bitboard_t	occupied = board->piecetype_bits[ALL_PIECES]
		^ square_bit(from) ^ square_bit(to);
	color_t		side_to_move = color_of_piece(piece_on(board, from));
	bitboard_t	attackers = attackers_list(board, to, occupied);
	bitboard_t	stm_attackers;
	bitboard_t	b;
	int			result = 1;

	while (true)
	{
		side_to_move = opposite_color(side_to_move);
		attackers &= occupied;

		if (!(stm_attackers = attackers & board->color_bits[side_to_move]))
			break ;

		if (board->stack->pinners[opposite_color(side_to_move)] & occupied)
		{
			stm_attackers &= ~board->stack->king_blockers[side_to_move];
			if (!stm_attackers)
				break ;
		}

		result ^= 1;

		if ((b = stm_attackers & board->piecetype_bits[PAWN]))
		{
			if ((next_score = PAWN_MG_SCORE - next_score) < result)
				break ;

			occupied ^= square_bit(first_square(b));
			attackers |= bishop_move_bits(to, occupied)
				& board_pieces(board, BISHOP, QUEEN);
		}
		else if ((b = stm_attackers & board->piecetype_bits[KNIGHT]))
		{
			if ((next_score = KNIGHT_MG_SCORE - next_score) < result)
				break ;

			occupied ^= square_bit(first_square(b));
		}
		else if ((b = stm_attackers & board->piecetype_bits[BISHOP]))
		{
			if ((next_score = BISHOP_MG_SCORE - next_score) < result)
				break ;

			occupied ^= square_bit(first_square(b));
			attackers |= bishop_move_bits(to, occupied)
				& board_pieces(board, BISHOP, QUEEN);
		}
		else if ((b = stm_attackers & board->piecetype_bits[ROOK]))
		{
			if ((next_score = ROOK_MG_SCORE - next_score) < result)
				break ;

			occupied ^= square_bit(first_square(b));
			attackers |= rook_move_bits(to, occupied)
				& board_pieces(board, ROOK, QUEEN);
		}
		else if ((b = stm_attackers & board->piecetype_bits[QUEEN]))
		{
			if ((next_score = QUEEN_MG_SCORE - next_score) < result)
				break ;

			occupied ^= square_bit(first_square(b));
			attackers |= bishop_move_bits(to, occupied)
				& board_pieces(board, BISHOP, QUEEN);
			attackers |= rook_move_bits(to, occupied)
				& board_pieces(board, ROOK, QUEEN);
		}
		else
			return ((attackers & ~board->color_bits[side_to_move])
				? result ^ 1 : result);
	}
	return (result);
}
