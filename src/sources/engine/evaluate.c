/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "engine.h"
#include "info.h"
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
	scorepair_t			ret = 0;

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
				return (0);
			else if (pieces == 2 && board_colored_pieces(board, WHITE, KNIGHT, BISHOP))
				return (0);
		}

		pieces = piece_count - pieces;

		if (eg < 0)
		{
			if (pieces == 1)
				return (0);
			else if (pieces == 2 && board_colored_pieces(board, BLACK, KNIGHT, BISHOP))
				return (0);
		}
	}


	if (piece_count <= 16)
		score = eg;
	else
		score = (eg * (32 - piece_count) + mg * (piece_count - 16)) / 16;

	return (Initiative + (board->side_to_move == WHITE ? score : -score));
}
