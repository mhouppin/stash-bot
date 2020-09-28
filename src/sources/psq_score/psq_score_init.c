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

#include "imath.h"
#include "psq_score.h"

scorepair_t		PsqScore[PIECE_NB][SQUARE_NB];
const score_t	PieceScores[PHASE_NB][PIECE_NB] = {
	{
		0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE,
		ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0,
		0, -PAWN_MG_SCORE, -KNIGHT_MG_SCORE, -BISHOP_MG_SCORE,
		-ROOK_MG_SCORE, -QUEEN_MG_SCORE, 0, 0
	},
	{
		0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE,
		ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0,
		0, -PAWN_EG_SCORE, -KNIGHT_EG_SCORE, -BISHOP_EG_SCORE,
		-ROOK_EG_SCORE, -QUEEN_EG_SCORE, 0, 0,
	}
};

#define S SPAIR

const scorepair_t	PawnBonus[RANK_NB][FILE_NB] = {
	{ },
	{ S(-13, 10), S(-17, -1), S(-17,  7), S(-11,  4), S(-14, 17), S( 40, 11), S( 32,-12), S(  4,-37) },
	{ S( -9,  2), S(-14,  0), S( -3, -6), S(  2, -6), S( 21,  4), S( 10,  4), S( 29,-15), S(  6,-21) },
	{ S( -7, 13), S(-10,  9), S(  2,-17), S(  9,-26), S( 19,-23), S( 24,-13), S(  7, -9), S( -1,-14) },
	{ S( -6, 40), S( -3, 29), S(  3,  5), S( 28,-31), S( 44,-25), S( 61,-17), S( 16,  6), S( -4,  6) },
	{ S( -3, 99), S( 23, 77), S( 39, 36), S( 35,-18), S( 55,-25), S(121, 12), S( 50, 32), S( 20, 39) },
	{ S( 70, 62), S( 59, 48), S( 51, 21), S( 51,-23), S( 44,-22), S( 31, -8), S( -9, -2), S(  2, 12) },
	{ }
};

const scorepair_t	PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
	{ },
	{ },
	
	// Knight

	{
		{ S( -75,-102), S( -17, -89), S( -18, -55), S( -21, -38) },
		{ S(  -4, -44), S( -25, -35), S( -12, -56), S(  -4, -30) },
		{ S( -16, -82), S(   2, -38), S(  -7, -21), S(  10,  12) },
		{ S(  11, -28), S(  25,  -7), S(  16,  33), S(  20,  38) },
		{ S(  32, -29), S(  24,   4), S(  50,  34), S(  35,  52) },
		{ S(   8, -50), S(  30, -19), S(  59,  29), S(  60,  20) },
		{ S(   3, -67), S(  -6, -38), S(  50, -43), S(  40,  12) },
		{ S( -75,-130), S( -12, -81), S( -19, -33), S(  -4, -21) }
	},

	// Bishop

	{
		{ S(  20, -28), S(  10, -23), S( -17, -28), S( -34, -24) },
		{ S(  20, -54), S(  18, -27), S(  12, -39), S(  -9, -20) },
		{ S(   7, -19), S(   6, -23), S(   2,  -7), S(   5,  -3) },
		{ S(   2, -31), S(   4, -15), S(  -1,   1), S(  25,   6) },
		{ S( -21,  -5), S(  18,   2), S(  17,  -5), S(  37,  -5) },
		{ S( -10,  -2), S(   9,   8), S(  38,   7), S(  25,  -2) },
		{ S( -51,  -9), S( -15,   0), S(  -8,  -5), S( -18,   1) },
		{ S( -43, -15), S(  -2, -13), S( -40, -13), S( -44,   1) }
	},

	// Rook

	{
		{ S( -23, -38), S( -15, -28), S(  -8, -25), S(  -4, -35) },
		{ S( -70, -34), S( -27, -47), S( -22, -46), S( -30, -43) },
		{ S( -42, -36), S( -21, -23), S( -44, -22), S( -34, -27) },
		{ S( -33,  -8), S( -30,   9), S( -40,  12), S( -27,   3) },
		{ S(  -8,  12), S(  11,  13), S(  16,  16), S(  34,  11) },
		{ S(  -9,  26), S(  46,  10), S(  34,  28), S(  52,  10) },
		{ S(  11,  34), S(  -7,  41), S(  27,  35), S(  34,  46) },
		{ S(  26,  52), S(  20,  56), S(   7,  57), S(  13,  47) }
	},

	// Queen

	{
		{ S(   6, -85), S(   0, -79), S(   5, -95), S(  16, -57) },
		{ S(   4, -66), S(  14, -82), S(  18, -98), S(   8, -50) },
		{ S(  -2, -52), S(   9, -38), S(   0, -17), S(  -6, -16) },
		{ S(   2, -27), S(   7,  -8), S(  -6,   2), S( -13,  44) },
		{ S(  17, -22), S(  13,  22), S(  -4,  18), S( -18,  52) },
		{ S(   8, -19), S(  23,  -8), S(   8,  20), S(   2,  24) },
		{ S(   6, -20), S(  15,  13), S(   9,  -3), S( -11,  21) },
		{ S(  24, -51), S(  26, -25), S(  28, -11), S(  14, -21) }
	},

	// King

	{
		{ S( 284, -63), S( 283,  35), S( 219,  60), S( 150,  55) },
		{ S( 284,  40), S( 263,  87), S( 205, 124), S( 185, 129) },
		{ S( 186,  86), S( 262, 106), S( 197, 143), S( 171, 166) },
		{ S( 152,  95), S( 224, 146), S( 192, 177), S( 157, 198) },
		{ S( 153, 124), S( 197, 181), S( 134, 205), S(  97, 208) },
		{ S( 131, 126), S( 163, 208), S( 104, 215), S(  45, 196) },
		{ S(  93,  56), S( 131, 178), S(  80, 163), S(  44, 152) },
		{ S(  59,  -3), S(  92,  63), S(  48,  78), S(   3,  84) }
	}
};

#undef S

void	psq_score_init(void)
{
	for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
	{
		scorepair_t		piece_value = create_scorepair(
			PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

		for (square_t square = SQ_A1; square <= SQ_H8; ++square)
		{
			scorepair_t		psq_entry;

			if (piece == WHITE_PAWN)
				psq_entry = piece_value + PawnBonus[rank_of_square(square)][file_of_square(square)];
			else
			{
				file_t	qside_file = min(file_of_square(square), file_of_square(square) ^ 7);

				psq_entry = piece_value + PieceBonus[piece][rank_of_square(square)][qside_file];
			}

			PsqScore[piece][square] = psq_entry;
			PsqScore[opposite_piece(piece)][opposite_square(square)] = -psq_entry;
		}
	}
}
