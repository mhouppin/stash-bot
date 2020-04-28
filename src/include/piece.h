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

#ifndef PIECE_H
# define PIECE_H

# include <stdint.h>
# include "color.h"
# include "score.h"

typedef int8_t	piece_t;
typedef int8_t	piecetype_t;

enum
{
	NO_PIECE,
	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN = 9,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
	PIECE_NB = 16
};

enum
{
	NO_PIECETYPE,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	ALL_PIECES = 0,
	PIECETYPE_NB = 8
};

INLINED piecetype_t	type_of_piece(piece_t piece)
{
	return (piece & 7);
}

INLINED color_t		color_of_piece(piece_t piece)
{
	return (piece >> 3);
}

INLINED piece_t		create_piece(color_t color, piecetype_t piecetype)
{
	return (piecetype + (color << 3));
}

INLINED piece_t		opposite_piece(piece_t piece)
{
	return (piece ^ 8);
}

#endif
