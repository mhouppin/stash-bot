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

#ifndef SCORE_H
# define SCORE_H

# include <stdint.h>
# include "inlining.h"

typedef int16_t		score_t;
typedef int32_t		scorepair_t;
typedef int16_t		phase_t;

enum
{
	DRAW = 0,
	VICTORY = 10000,
	MATE_FOUND = 31000,
	MATE = 32000,
	INF_SCORE = 32001,
	NO_SCORE = 32002
};

enum
{
	NO_BOUND = 0,
	UPPER_BOUND,
	LOWER_BOUND,
	EXACT_BOUND
};

enum
{
	MIDGAME,
	ENDGAME,
	PHASE_NB
};

INLINED score_t		midgame_score(scorepair_t pair)
{
	return ((score_t)(uint16_t)(((uint32_t)pair + 32768) >> 16));
}

INLINED score_t		endgame_score(scorepair_t pair)
{
	return ((score_t)(uint16_t)(uint32_t)pair);
}

#define SPAIR(mg, eg) ((scorepair_t)((uint32_t)(mg) << 16) + (eg))

INLINED scorepair_t	create_scorepair(score_t midgame, score_t endgame)
{
	return ((scorepair_t)((uint32_t)midgame << 16) + endgame);
}

INLINED scorepair_t	scorepair_multiply(scorepair_t s, int i)
{
	return (s * i);
}

INLINED scorepair_t	scorepair_divide(scorepair_t s, int i)
{
	return (create_scorepair(midgame_score(s) / i, endgame_score(s) / i));
}

INLINED score_t		mate_in(int ply)
{
	return (MATE - ply);
}

INLINED score_t		mated_in(int ply)
{
	return (ply - MATE);
}

#endif
