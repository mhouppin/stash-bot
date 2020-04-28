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

#ifndef CASTLING_H
# define CASTLING_H

# include "color.h"
# include "inlining.h"

enum
{
	WHITE_OO = 1,
	WHITE_OOO = 2,
	WHITE_CASTLING = 3,
	BLACK_OO = 4,
	KINGSIDE_CASTLING = 5,
	BLACK_OOO = 8,
	QUEENSIDE_CASTLING = 10,
	BLACK_CASTLING = 12,
	ANY_CASTLING = 15,
	CASTLING_NB = 16
};

INLINED int	has_castling(color_t color, int castlings)
{
	return (castlings & (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING));
}

#endif
