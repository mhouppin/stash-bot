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
	CastlingBonus = SPAIR(76, -56),
	Initiative = SPAIR(12, 20),

	MinorWeight = 31,
	RookWeight = 20,
	QueenWeight = 95,
	SafetyRatio = SPAIR(1, 1),

	BishopPairBonus = SPAIR(20, 128),
	KnightPairPenalty = SPAIR(-5, 0),
	RookPairPenalty = SPAIR(-24, -4),
	NonPawnBonus = SPAIR(96, 165),

	RookOnSemiOpenFile = SPAIR(23, 23),
	RookOnOpenFile = SPAIR(50, 20),
	RookXrayQueen = SPAIR(7, 19),

	QueenPhase = 4,
	RookPhase = 2,
	MinorPhase = 1,

	MidgamePhase = 24,
};

const scorepair_t	MobilityN[9] = {
	SPAIR( -74, -72), SPAIR( -58, -68), SPAIR( -26, -60), SPAIR( -16,  -7),
	SPAIR(  -3,   2), SPAIR(  -3,  27), SPAIR(   6,  31), SPAIR(  16,  33),
	SPAIR(  32,  35)
};

const scorepair_t	MobilityB[14] = {
	SPAIR( -83,-119), SPAIR( -61,-114), SPAIR( -17,-104), SPAIR( -15, -55),
	SPAIR(  -4, -26), SPAIR(   4,  -5), SPAIR(   6,  18), SPAIR(   6,  29),
	SPAIR(   6,  40), SPAIR(   6,  46), SPAIR(  10,  44), SPAIR(  23,  42),
	SPAIR(  32,  39), SPAIR(  36,  36)
};

const scorepair_t	MobilityR[15] = {
	SPAIR( -42, -20), SPAIR( -41, -12), SPAIR( -38,  -5), SPAIR( -36,  28),
	SPAIR( -35,  74), SPAIR( -34,  88), SPAIR( -33, 108), SPAIR( -29, 117),
	SPAIR( -25, 122), SPAIR( -20, 127), SPAIR( -15, 135), SPAIR( -14, 138),
	SPAIR(  -8, 139), SPAIR(   9, 140), SPAIR(  46, 141)
};

const scorepair_t	MobilityQ[28] = {
	SPAIR(  92,  13), SPAIR(  94,  67), SPAIR(  96, 121), SPAIR(  97, 175),
	SPAIR(  99, 229), SPAIR( 101, 285), SPAIR( 103, 347), SPAIR( 105, 421),
	SPAIR( 107, 481), SPAIR( 109, 523), SPAIR( 113, 544), SPAIR( 115, 573),
	SPAIR( 119, 587), SPAIR( 123, 590), SPAIR( 122, 602), SPAIR( 121, 607),
	SPAIR( 120, 608), SPAIR( 119, 615), SPAIR( 118, 614), SPAIR( 117, 611),
	SPAIR( 117, 601), SPAIR( 116, 597), SPAIR( 116, 592), SPAIR( 115, 590),
	SPAIR( 115, 588), SPAIR( 114, 587), SPAIR( 114, 585), SPAIR( 114, 584)
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
	bitboard_t		safe;

	const bitboard_t	occupancy = board->piecetype_bits[ALL_PIECES];

	if (c == WHITE)
	{
		king_zone = king_moves(board->piece_list[BLACK_KING][0]);
		king_zone |= shift_down(king_zone);
		safe = ~black_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[BLACK]);
	}
	else
	{
		king_zone = king_moves(board->piece_list[WHITE_KING][0]);
		king_zone |= shift_up(king_zone);
		safe = ~white_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[WHITE]);
	}

	// Exclude unsafe squares from king zone target squares

	king_zone &= safe;

	const square_t *list = board->piece_list[create_piece(c, KNIGHT)];

	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = knight_moves(sq);

		ret += MobilityN[popcount(b & safe)];

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

		ret += MobilityB[popcount(b & safe)];

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

		ret += MobilityR[popcount(b & safe)];

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

		ret += MobilityQ[popcount(b & safe)];

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * QueenWeight;
		}
	}

	if (attackers < 8)
		wattacks = wattacks * AttackWeights[attackers] / 100;

	ret += SafetyRatio * wattacks;

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
	eval += (board->side_to_move == WHITE) ? Initiative : -Initiative;

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

	{
		int		phase = QueenPhase * popcount(board->piecetype_bits[QUEEN])
			+ RookPhase * popcount(board->piecetype_bits[ROOK])
			+ MinorPhase * popcount(board->piecetype_bits[KNIGHT] | board->piecetype_bits[BISHOP]);

		if (phase >= MidgamePhase)
			score = mg;
		else
		{
			score = mg * phase / MidgamePhase;
			score += eg * (MidgamePhase - phase) / MidgamePhase;
		}
	}

	return (board->side_to_move == WHITE ? score : -score);
}
