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

#include "movelist.h"
#include <unistd.h>

void	sort_moves(extmove_t *begin, extmove_t *end)
{
	const ssize_t	size = (ssize_t)(end - begin);

	for (ssize_t i = 1; i < size; ++i)
	{
		extmove_t	tmp = begin[i];
		ssize_t		j = i - 1;
		while (j >= 0 && begin[j].score < tmp.score)
		{
			begin[j + 1] = begin[j];
			--j;
		}
		begin[j + 1] = tmp;
	}
}
