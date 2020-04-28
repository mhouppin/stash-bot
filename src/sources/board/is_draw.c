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

#include "board.h"
#include "movelist.h"

bool	is_draw(const board_t *board, int ply)
{
	if (board->stack->rule50 > 99)
	{
		if (!board->stack->checkers)
			return (true);

		movelist_t	movelist;

		list_all(&movelist, board);

		if (movelist_size(&movelist) != 0)
			return (true);
	}

	if (board->stack->repetition && board->stack->repetition < ply)
		return (true);

	return (false);
}
