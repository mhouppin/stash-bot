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

#ifndef HISTORY_H
# define HISTORY_H

# include <string.h>
# include "move.h"
# include "piece.h"
# include "score.h"

enum
{
	HistoryBonus = 256
};

extern uint64_t		g_butterfly_table[PIECE_NB][SQUARE_NB * SQUARE_NB];
extern uint64_t		g_history_table[PIECE_NB][SQUARE_NB * SQUARE_NB];

INLINED void	reset_history(void)
{
	memset(g_butterfly_table, 0, sizeof(g_butterfly_table));
	memset(g_history_table, 0, sizeof(g_history_table));
}

INLINED void	add_hist_bonus(piece_t piece, move_t move)
{
	g_history_table[piece][move_squares(move)] += 1;
}

INLINED void	add_hist_penalty(piece_t piece, move_t move)
{
	g_butterfly_table[piece][move_squares(move)] += 1;
}

INLINED score_t	get_hist_score(piece_t piece, move_t move)
{
	const int	idx = move_squares(move);

	// Small trick to catch zero-division error and history score overflow.

	if (g_history_table[piece][idx] >= g_butterfly_table[piece][idx])
		return (HistoryBonus);

	return (g_history_table[piece][idx] * HistoryBonus
		/ g_butterfly_table[piece][idx]);
}

#endif
