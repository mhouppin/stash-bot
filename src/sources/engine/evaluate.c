/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   evaluate.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 22:28:26 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/02 12:04:06 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "pawns.h"

enum
{
	CastlingBonus = SPAIR(100, 0),
	Initiative = 15,
	MobilityBase = SPAIR(-42, -66),
	MobilityPlus = SPAIR(7, 11),
	MobilityMax = SPAIR(21, 33)
};

scorepair_t	evaluate_mobility(const board_t *board, color_t c)
{
	scorepair_t	ret = 0;

	const square_t *list = board->piece_list[create_piece(c, BISHOP)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		int move_count = popcount(bishop_moves(board, sq));

		if (move_count >= 9)
			ret += MobilityMax;
		else
			ret += MobilityBase + MobilityPlus * move_count;
	}

	list = board->piece_list[create_piece(c, ROOK)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		int move_count = popcount(rook_moves(board, sq));

		if (move_count >= 9)
			ret += MobilityMax;
		else
			ret += MobilityBase + MobilityPlus * move_count;
	}

	list = board->piece_list[create_piece(c, QUEEN)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		int move_count = popcount(queen_moves(board, sq));

		if (move_count >= 9)
			ret += MobilityMax;
		else
			ret += MobilityBase + MobilityPlus * move_count;
	}

	return (ret);
}

score_t		evaluate(const board_t *board)
{
	scorepair_t		eval = board->psq_scorepair;

	if (board->stack->castlings & WHITE_CASTLING)
		eval += CastlingBonus;
	if (board->stack->castlings & BLACK_CASTLING)
		eval -= CastlingBonus;

	eval += evaluate_pawns(board);
	eval += evaluate_mobility(board, WHITE);
	eval -= evaluate_mobility(board, BLACK);

	score_t		mg = midgame_score(eval);
	score_t		eg = endgame_score(eval);
	int			piece_count = popcount(board->piecetype_bits[ALL_PIECES]);
	score_t		score;

	if (piece_count <= 7)
	{
		// Insufficient material check.

		int		pieces = popcount(board->color_bits[WHITE]);

		if (eg > 0)
		{
			if (pieces == 1)
				eg = 0;
			else if (pieces == 2 && board_colored_pieces(board, WHITE, KNIGHT, BISHOP))
				eg = 0;
		}

		pieces = piece_count - pieces;

		if (eg < 0)
		{
			if (pieces == 1)
				eg = 0;
			else if (pieces == 2 && board_colored_pieces(board, BLACK, KNIGHT, BISHOP))
				eg = 0;
		}
	}

	if (piece_count <= 16)
		score = eg;
	else
		score = (eg * (32 - piece_count) + mg * (piece_count - 16)) / 16;

	return (Initiative + (board->side_to_move == WHITE ? score : -score));
}
