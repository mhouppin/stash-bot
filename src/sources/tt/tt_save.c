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

#include "tt.h"

void		tt_save(tt_entry_t *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m)
{
	if (m || k != entry->key)
		entry->bestmove = (uint16_t)m;

	if (k != entry->key || d - DEPTH_OFFSET > entry->depth || b == EXACT_BOUND)
	{
		extern transposition_t	g_hashtable;

		entry->key = k;
		entry->score = s;
		entry->eval = e;
		entry->genbound = g_hashtable.generation | (uint8_t)b;
		entry->depth = (d - DEPTH_OFFSET);
	}
}
