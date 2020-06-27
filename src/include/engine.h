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

#ifndef ENGINE_H
# define ENGINE_H

# include <time.h>
# include "board.h"

typedef struct
{
	int		plies;
	score_t	static_eval;
	move_t	killers[2];
	move_t	*pv;
}
searchstack_t;

typedef struct
{
	move_t	move;
	int		depth;
	score_t	previous_score;
	score_t	score;
	move_t	pv[512];
}
root_move_t;

// All search components are here
enum
{
	NMP_MinDepth = 3,
	NMP_MinPlies = 1,
	NMP_BaseReduction = 3,
	NMP_EvalScale = 256,
	NMP_MaxEvalReduction = 3,
	NMP_TrustDepth = 10,

	LMR_MinDepth = 3,
	LMR_MinMoves = 4,

	Razor_LightMargin = 150,
	Razor_HeavyMargin = 300
};

void	sort_root_moves(root_move_t *begin, root_move_t *end);

void	engine_go(board_t *board);
void	search_bestmove(board_t *board, int depth, root_move_t *begin,
		root_move_t *end, int pv_line);
score_t	qsearch(board_t *board, int depth, score_t alpha, score_t beta,
		searchstack_t *ss);
score_t	search(board_t *board, int depth, score_t alpha, score_t beta,
		searchstack_t *ss);

bool	out_of_time(void);
score_t	evaluate(const board_t *board);

#endif
