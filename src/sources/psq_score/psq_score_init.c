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

#include "psq_score.h"

scorepair_t		PsqScore[PIECE_NB][SQUARE_NB];
score_t			PieceScores[PHASE_NB][PIECE_NB] = {
	{
		0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE,
		ROOK_MG_SCORE, QUEEN_MG_SCORE, 0
	},
	{
		0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE,
		ROOK_EG_SCORE, QUEEN_EG_SCORE, 0
	}
};

const score_t	SquareScores[PHASE_NB][PIECETYPE_NB][SQUARE_NB] = {
	{
		{0},
		{
			0,		0,		0,		0,		0,		0,		0,		0,
			0,		0,		0,		-10,	-15,	0,		0,		0,
			5,		10,		5,		10,		15,		10,		10,		5,
			5,		5,		10,		30,		40,		15,		5,		5,
			10,		10,		10,		35,		45,		15,		10,		10,
			20,		15,		20,		40,		45,		25,		15,		20,
			35,		30,		35,		45,		50,		35,		30,		35,
			0,		0,		0,		0,		0,		0,		0,		0
		},
		{
			-60,	-20,	-10,	0,		0,		-10,	-20,	-60,
			-45,	-25,	10,		15,		15,		10,		-25,	-45,
			-5,		15,		20,		20,		20,		20,		15,		-5,
			5,		20,		25,		30,		30,		25,		20,		5,
			0,		10,		20,		35,		35,		20,		10,		0,
			10,		15,		20,		40,		40,		20,		15,		10,
			-5,		5,		10,		25,		25,		10,		5,		-5,
			-30,	-10,	-5,		0,		0,		-5,		-10,	-30
		},
		{
			-55,	-5,		-10,	-25,	-25,	-10,	-5,		-55,
			-15,	10,		20,		5,		5,		20,		10,		-15,
			-5,		20,		-5,		15,		15,		-5,		20,		-5,
			-5,		10,		25,		40,		40,		25,		10,		-5,
			-10,	30,		20,		30,		30,		20,		30,		-10,
			-15,	5,		0,		10,		10,		0,		5,		-15,
			-15,	-15,	5,		0,		0,		5,		-15,	-15,
			-50,	0,		-15,	-25,	-25,	-15,	0,		-50
		},
		{
			-30,	-20,	-15,	-5,		-5,		-15,	-20,	-30,
			-20,	-15,	-10,	5,		5,		-10,	-15,	-20,
			-25,	-10,	0,		5,		5,		0,		-10,	-25,
			-15,	-5,		-5,		-5,		-5,		-5,		-5,		-15,
			-25,	-15,	-5,		5,		5,		-5,		-15,	-25,
			-20,	0,		5,		10,		10,		5,		0,		-20,
			0,		10,		15,		20,		20,		15,		10,		0,
			-15,	-20,	0,		10,		10,		0,		-20,	-15
		},
		{
			5,		-5,		-5,		5,		5,		-5,		-5,		5,
			-5,		5,		10,		10,		10,		10,		5,		-5,
			-5,		5,		15,		5,		5,		15,		5,		-5,
			5,		5,		10,		10,		10,		10,		5,		5,
			0,		15,		10,		5,		5,		10,		15,		0,
			-5,		10,		5,		10,		10,		5,		10,		-5,
			-5,		5,		10,		10,		10,		10,		5,		-5,
			0,		0,		0,		0,		0,		0,		0,		0
		},
		{
			270,	325,	270,	190,	190,	270,	325,	270,
			280,	300,	230,	175,	175,	230,	300,	280,
			195,	260,	170,	120,	120,	170,	260,	195,
			165,	190,	140,	100,	100,	140,	190,	165,
			155,	180,	105,	70,		70,		105,	180,	155,
			125,	145,	80,		30,		30,		80,		145,	125,
			90,		120,	65,		35,		35,		65,		120,	90,
			60,		90,		45,		0,		0,		45,		90,		60
		},
		{0}
	},
	{
		{0},
		{
			0,		0,		0,		0,		0,		0,		0,		0,
			-10,	-5,		10,		0,		15,		5,		-5,		-20,
			-10,	-10,	-10,	5,		5,		5,		-5,		-5,
			5,		0,		-10,	-5,		-15,	-10,	-10,	-10,
			10,		5,		5,		-10,	-10,	-5,		15,		10,
			30,		20,		20,		30,		30,		5,		5,		15,
			0,		-10,	10,		20,		25,		20,		5,		5,
			0,		0,		0,		0,		0,		0,		0,		0
		},
		{
			-95,	-65,	-50,	-20,	-20,	-50,	-65,	-95,
			-65,	-55,	-20,	10,		10,		-20,	-55,	-65,
			-40,	-25,	-10,	30,		30,		-10,	-25,	-40,
			-35,	0,		15,		30,		30,		15,		0,		-35,
			-45,	-15,	10,		40,		40,		10,		-15,	-45,
			-50,	-45,	-15,	15,		15,		-15,	-45,	-50,
			-70,	-50,	-50,	10,		10,		-50,	-50,	-70,
			-100,	-90,	-55,	-15,	-15,	-55,	-90,	-100
		},
		{
			-55,	-30,	-35,	-10,	-10,	-35,	-30,	-55,
			-35,	-15,	-15,	0,		0,		-15,	-15,	-35,
			-15,	0,		0,		10,		10,		0,		0,		-15,
			-20,	-5,		0,		15,		15,		0,		-5,		-20,
			-15,	0,		-15,	15,		15,		-15,	0,		-15,
			-30,	5,		5,		5,		5,		5,		5,		-30,
			-30,	-20,	0,		0,		0,		0,		-20,	-30,
			-45,	-40,	-35,	-25,	-25,	-35,	-40,	-45
		},
		{
			-10,	-15,	-10,	-10,	-10,	-10,	-15,	-10,
			-10,	-10,	0,		0,		0,		0,		-10,	-10,
			5,		-10,	0,		-5,		-5,		0,		-10,	5,
			-5,		0,		-10,	5,		5,		-10,	0,		-5,
			-5,		10,		5,		-5,		-5,		5,		10,		-5,
			5,		0,		-5,		10,		10,		-5,		0,		5,
			5,		5,		20,		-5,		-5,		20,		5,		5,
			20,		0,		20,		15,		15,		20,		0,		20
		},
		{
			-70,	-55,	-45,	-25,	-25,	-45,	-55,	-70,
			-55,	-30,	-20,	-5,		-5,		-20,	-30,	-55,
			-40,	-20,	-10,	5,		5,		-10,	-20,	-40,
			-25,	-5,		15,		25,		25,		15,		-5,		-25,
			-30,	-5,		10,		20,		20,		10,		-5,		-30,
			-40,	-20,	-10,	0,		0,		-10,	-20,	-40,
			-50,	-25,	-25,	-10,	-10,	-25,	-25,	-50,
			-75,	-50,	-45,	-35,	-35,	-45,	-50,	-75
		},
		{
			0,		45,		85,		75,		75,		85,		45,		0,
			55,		100,	135,	135,	135,	135,	100,	55,
			90,		130,	170,	175,	175,	170,	130,	90,
			105,	155,	170,	170,	170,	170,	155,	105,
			95,		165,	200,	200,	200,	200,	165,	95,
			90,		170,	185,	190,	190,	185,	170,	90,
			45,		120,	115,	130,	130,	115,	120,	45,
			10,		60,		75,		80,		80,		75,		60,		10
		},
		{0}
	}
};

void	psq_score_init(void)
{
	for (piece_t piece = BLACK_PAWN; piece <= BLACK_KING; ++piece)
	{
		PieceScores[MIDGAME][piece] =
			-PieceScores[MIDGAME][opposite_color(piece)];
		PieceScores[ENDGAME][piece] =
			-PieceScores[ENDGAME][opposite_color(piece)];
	}

	for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
		for (square_t square = SQ_A1; square <= SQ_H8; ++square)
		{
			score_t midgame_score = PieceScores[MIDGAME][piece]
				+ SquareScores[MIDGAME][piece][square];

			score_t endgame_score = PieceScores[ENDGAME][piece]
				+ SquareScores[ENDGAME][piece][square];

			scorepair_t	wpsq = create_scorepair(midgame_score, endgame_score);
			scorepair_t	bpsq = create_scorepair(-midgame_score, -endgame_score);

			PsqScore[piece][square] = wpsq;
			PsqScore[opposite_piece(piece)][opposite_square(square)] = bpsq;
		}
}
