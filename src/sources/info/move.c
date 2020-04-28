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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "info.h"

const char	*move_to_str(move_t move, bool is_chess960)
{
	static char		buf[6];

	if (move == NO_MOVE)
		return ("none");

	if (move == NULL_MOVE)
		return ("0000");

	square_t		from = move_from_square(move);
	square_t		to = move_to_square(move);

	if (type_of_move(move) == CASTLING && !is_chess960)
		to = create_square(to > from ? FILE_G : FILE_C, rank_of_square(from));

	buf[0] = file_of_square(from) + 'a';
	buf[1] = rank_of_square(from) + '1';
	buf[2] = file_of_square(to) + 'a';
	buf[3] = rank_of_square(to) + '1';

	if (type_of_move(move) == PROMOTION)
	{
		buf[4] = " pnbrqk"[promotion_type(move)];
		buf[5] = '\0';
	}
	else
		buf[4] = '\0';

	return (buf);
}

move_t		str_to_move(const board_t *board, const char *str)
{
	char	*trick = strdup(str);

	if (strlen(str) == 5)
		trick[4] = tolower(trick[4]);

	movelist_t	movelist;

	list_all(&movelist, board);

	for (const extmove_t *m = movelist_begin(&movelist);
		m < movelist_end(&movelist); ++m)
	{
		const char	*s = move_to_str(m->move, board->chess960);
		if (!strcmp(trick, s))
		{
			free(trick);
			return (m->move);
		}
	}

	free(trick);
	return (NO_MOVE);
}
