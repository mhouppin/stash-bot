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

#ifndef PSQ_SCORE_H
# define PSQ_SCORE_H

# include "piece.h"
# include "score.h"
# include "square.h"

enum
{
	PAWN_MG_SCORE = 101,
	KNIGHT_MG_SCORE = 391,
	BISHOP_MG_SCORE = 411,
	ROOK_MG_SCORE = 598,
	QUEEN_MG_SCORE = 1116,

	PAWN_EG_SCORE = 223,
	KNIGHT_EG_SCORE = 765,
	BISHOP_EG_SCORE = 793,
	ROOK_EG_SCORE = 1207,
	QUEEN_EG_SCORE = 2226
};

extern const score_t	PieceScores[PHASE_NB][PIECE_NB];
extern scorepair_t		PsqScore[PIECE_NB][SQUARE_NB];

#endif
