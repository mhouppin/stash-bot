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
#include "imath.h"
#include "info.h"
#include "pawns.h"

enum
{
	CastlingBonus = SPAIR(100, 0),
	Initiative = 15,
	MobilityBase = SPAIR(-42, -66),
	MobilityPlus = SPAIR(7, 11),

	PawnWeight = 10,
	MinorWeight = 20,
	RookWeight = 40,
	QueenWeight = 80,
	SafetyScale = 2,
	SafetyRatio = SPAIR(2, 1),

	BishopPairBonus = SPAIR(30, 50),
	KnightPairPenalty = SPAIR(-15, -35),
	RookPairPenalty = SPAIR(-20, -40),
	NonPawnBonus = SPAIR(32, 48),

	RookOnSemiOpenFile = SPAIR(24, 24),
	RookOnOpenFile = SPAIR(48, 6),
	RookXrayQueen = SPAIR(18, 0)
};

const int	AttackWeights[8] = {
	0, 0, 50, 75, 88, 94, 97, 99
};

scorepair_t	evaluate_rook_patterns(const board_t *board, color_t c)
{
	scorepair_t	ret = 0;

	bitboard_t	my_pawns = board->piecetype_bits[PAWN] & board->color_bits[c];
	bitboard_t	their_pawns = board->piecetype_bits[PAWN] & ~my_pawns;
	bitboard_t	their_queens = board->piecetype_bits[QUEEN]
		& board->color_bits[opposite_color(c)];

	const square_t	*list = board->piece_list[create_piece(c, ROOK)];

	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	rook_file = file_square_bits(sq);

		if (!(rook_file & my_pawns))
			ret += (rook_file & their_pawns) ? RookOnSemiOpenFile : RookOnOpenFile;

		if (rook_file & their_queens)
			ret += RookXrayQueen;
	}
	return (ret);
}

scorepair_t	evaluate_material(const board_t *board, color_t c)
{
	scorepair_t			ret = 0;
	const bitboard_t	b = board->color_bits[c];

	if (more_than_one(b & board->piecetype_bits[BISHOP]))
		ret += BishopPairBonus;

	if (more_than_one(b & board->piecetype_bits[KNIGHT]))
		ret += KnightPairPenalty;

	if (more_than_one(b & board->piecetype_bits[ROOK]))
		ret += RookPairPenalty;

	ret += NonPawnBonus * popcount(b & ~board->piecetype_bits[PAWN]);

	return (ret);
}

scorepair_t	evaluate_mobility(const board_t *board, color_t c)
{
	scorepair_t		ret = 0;
	int				wattacks = 0;
	int				attackers = 0;
	bitboard_t		king_zone;

	const bitboard_t	occupancy = board->piecetype_bits[ALL_PIECES];

	if (c == WHITE)
	{
		king_zone = king_moves(board->piece_list[BLACK_KING][0]);
		king_zone |= shift_down(king_zone);
		king_zone &= ~black_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[BLACK]);
	}
	else
	{
		king_zone = king_moves(board->piece_list[WHITE_KING][0]);
		king_zone |= shift_up(king_zone);
		king_zone &= ~white_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[WHITE]);
	}
	
	const square_t *list = board->piece_list[create_piece(c, KNIGHT)];

	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = knight_moves(sq);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * MinorWeight;
		}
	}

	list = board->piece_list[create_piece(c, BISHOP)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = bishop_move_bits(sq, occupancy);

		int			move_count = popcount(b);

		ret += MobilityBase + MobilityPlus * min(move_count, 9);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * MinorWeight;
		}
	}

	list = board->piece_list[create_piece(c, ROOK)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = rook_move_bits(sq, occupancy);

		int			move_count = popcount(b);

		ret += MobilityBase + MobilityPlus * min(move_count, 9);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * RookWeight;
		}
	}

	list = board->piece_list[create_piece(c, QUEEN)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = bishop_move_bits(sq, occupancy)
			| rook_move_bits(sq, occupancy);

		int			move_count = popcount(b);

		ret += MobilityBase + MobilityPlus * min(move_count, 9);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * QueenWeight;
		}
	}

	if (attackers >= 8)
		ret += scorepair_divide(SafetyRatio * wattacks, SafetyScale);
	else
		ret += scorepair_divide(SafetyRatio * (wattacks * AttackWeights[attackers] / 100), SafetyScale);

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
	eval += evaluate_material(board, WHITE);
	eval -= evaluate_material(board, BLACK);
	eval += evaluate_mobility(board, WHITE);
	eval -= evaluate_mobility(board, BLACK);
	eval += evaluate_rook_patterns(board, WHITE);
	eval -= evaluate_rook_patterns(board, BLACK);

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
