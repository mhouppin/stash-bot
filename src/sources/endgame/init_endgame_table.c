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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "endgame.h"

endgame_entry_t	g_endgame_table[EndgameSize];

void	add_endgame(hashkey_t k, endgame_eval_t e)
{
	endgame_entry_t		*entry = &g_endgame_table[k & (EndgameSize - 1)];

	if (entry->key)
	{
		fprintf(stderr, "Endgame table too small\n");
		exit(EXIT_FAILURE);
	}
	entry->key = k;
	entry->eg_eval = e;
}

void	init_endgame_table(void)
{
	hashkey_t	kk = ZobristPsq[WHITE_KING][0] ^ ZobristPsq[BLACK_KING][0];
	hashkey_t	wn = ZobristPsq[WHITE_KNIGHT][0];
	hashkey_t	wb = ZobristPsq[WHITE_BISHOP][0];
	hashkey_t	bn = ZobristPsq[BLACK_KNIGHT][0];
	hashkey_t	bb = ZobristPsq[BLACK_BISHOP][0];
	hashkey_t	wnn = wn ^ ZobristPsq[WHITE_KNIGHT][1];
	hashkey_t	bnn = bn ^ ZobristPsq[BLACK_KNIGHT][1];
	hashkey_t	wr = ZobristPsq[WHITE_ROOK][0];
	hashkey_t	br = ZobristPsq[BLACK_ROOK][0];
	hashkey_t	wq = ZobristPsq[WHITE_QUEEN][0];
	hashkey_t	bq = ZobristPsq[BLACK_QUEEN][0];

	// Insufficient material draws

	add_endgame(kk, &eval_draw);
	add_endgame(kk ^ wn, &eval_draw);
	add_endgame(kk ^ wb, &eval_draw);
	add_endgame(kk ^ bn, &eval_draw);
	add_endgame(kk ^ bb, &eval_draw);
	add_endgame(kk ^ wnn, &eval_draw);
	add_endgame(kk ^ bnn, &eval_draw);
	add_endgame(kk ^ wnn ^ bn, &eval_draw);
	add_endgame(kk ^ wnn ^ bb, &eval_draw);
	add_endgame(kk ^ bnn ^ wn, &eval_draw);
	add_endgame(kk ^ bnn ^ bb, &eval_draw);
	add_endgame(kk ^ wn ^ wb, &eval_knbk);
	add_endgame(kk ^ bn ^ bb, &eval_knbk);
	add_endgame(kk ^ wb ^ ZobristPsq[WHITE_BISHOP][1], &eval_kbbk);
	add_endgame(kk ^ bb ^ ZobristPsq[BLACK_BISHOP][1], &eval_kbbk);
	add_endgame(kk ^ wr, &eval_krk);
	add_endgame(kk ^ br, &eval_krk);
	add_endgame(kk ^ wq ^ br, &eval_kqkr);
	add_endgame(kk ^ bq ^ wr, &eval_kqkr);
	add_endgame(kk ^ wr ^ wb, &eval_krbk);
	add_endgame(kk ^ br ^ bb, &eval_krbk);
	add_endgame(kk ^ wq ^ bn, &eval_kqkm);
	add_endgame(kk ^ wq ^ bb, &eval_kqkm);
	add_endgame(kk ^ bq ^ wn, &eval_kqkm);
	add_endgame(kk ^ bq ^ wb, &eval_kqkm);
	add_endgame(kk ^ wq, &eval_kqk);
	add_endgame(kk ^ bq, &eval_kqk);
}
