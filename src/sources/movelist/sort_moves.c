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
	const size_t	size = (size_t)(end - begin);

	for (size_t gap = size / 2; gap > 0; gap /= 2)
		for (size_t start = gap; start < size; ++start)
			for (ssize_t i = (ssize_t)(start - gap); i >= 0; i -= gap)
			{
				if (begin[i + gap].score <= begin[i].score)
					break ;
				else
				{
					extmove_t	tmp = begin[i + gap];
					begin[i + gap] = begin[i];
					begin[i] = tmp;
				}
			}
}
