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

#ifndef ENDGAME_H
# define ENDGAME_H

# include <stdlib.h>
# include "board.h"

enum
{
	EndgameSize = 2048,

	KNBK_Bonus = 1000,
	KBBK_Bonus = 1500,
	KRK_Bonus = 2000,
	KQKR_Bonus = 2500,
	KRBK_Bonus = 2800,
	KQKM_Bonus = 3000,
	KQK_Bonus = 3500
};

typedef score_t		(*endgame_eval_t)(const board_t *board);

typedef struct
{
	hashkey_t		key;
	endgame_eval_t	eg_eval;
}
endgame_entry_t;

extern endgame_entry_t		g_endgame_table[EndgameSize];

INLINED endgame_entry_t		*endgame_probe(const board_t *board)
{
	return (&g_endgame_table[board->stack->material_key & (EndgameSize - 1)]);
}

INLINED score_t		proximity_bonus(square_t sq1, square_t sq2)
{
	return (112 - SquareDistance[sq1][sq2] * 16);
}

INLINED score_t		cornered_bonus(square_t k)
{
	int		edge_rank = rank_of_square(k);
	int		edge_file = file_of_square(k);

	if (edge_rank >= 4)
		edge_rank = 7 - edge_rank;
	if (edge_file >= 4)
		edge_file = 7 - edge_file;
	return (576 - 32 * edge_rank * edge_rank - 32 * edge_file * edge_file);
}

void	init_endgame_table(void);

score_t	eval_draw(const board_t *board);
score_t	eval_knbk(const board_t *board);
score_t	eval_kbbk(const board_t *board);
score_t	eval_krk(const board_t *board);
score_t	eval_kqkr(const board_t *board);
score_t	eval_krbk(const board_t *board);
score_t	eval_kqkm(const board_t *board);
score_t	eval_kqk(const board_t *board);

#endif
