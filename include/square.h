/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   square.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 14:29:22 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 16:04:10 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef SQUARE_H
# define SQUARE_H

# include <stdbool.h>
# include <stdint.h>
# include "inlining.h"
# include "color.h"

typedef int16_t	square_t;
typedef int16_t	direction_t;
typedef int8_t	file_t;
typedef int8_t	rank_t;

enum
{
	SQ_A1,	SQ_B1,	SQ_C1,	SQ_D1,	SQ_E1,	SQ_F1,	SQ_G1,	SQ_H1,
	SQ_A2,	SQ_B2,	SQ_C2,	SQ_D2,	SQ_E2,	SQ_F2,	SQ_G2,	SQ_H2,
	SQ_A3,	SQ_B3,	SQ_C3,	SQ_D3,	SQ_E3,	SQ_F3,	SQ_G3,	SQ_H3,
	SQ_A4,	SQ_B4,	SQ_C4,	SQ_D4,	SQ_E4,	SQ_F4,	SQ_G4,	SQ_H4,
	SQ_A5,	SQ_B5,	SQ_C5,	SQ_D5,	SQ_E5,	SQ_F5,	SQ_G5,	SQ_H5,
	SQ_A6,	SQ_B6,	SQ_C6,	SQ_D6,	SQ_E6,	SQ_F6,	SQ_G6,	SQ_H6,
	SQ_A7,	SQ_B7,	SQ_C7,	SQ_D7,	SQ_E7,	SQ_F7,	SQ_G7,	SQ_H7,
	SQ_A8,	SQ_B8,	SQ_C8,	SQ_D8,	SQ_E8,	SQ_F8,	SQ_G8,	SQ_H8,
	SQ_NONE,
	SQUARE_NB = 64
};

enum
{
	NORTH = 8,
	SOUTH = -8,
	EAST = 1,
	WEST = -1,
	NORTH_EAST = NORTH + EAST,
	NORTH_WEST = NORTH + WEST,
	SOUTH_EAST = SOUTH + EAST,
	SOUTH_WEST = SOUTH + WEST
};

enum
{
	FILE_A,	FILE_B,	FILE_C, FILE_D,	FILE_E,	FILE_F,	FILE_G,	FILE_H,	FILE_NB
};

enum
{
	RANK_1,	RANK_2,	RANK_3,	RANK_4,	RANK_5,	RANK_6,	RANK_7,	RANK_8,	RANK_NB
};

extern int	SquareDistance[SQUARE_NB][SQUARE_NB];

INLINED file_t	file_of_square(square_t square)
{
	return (square & 7);
}

INLINED rank_t	rank_of_square(square_t square)
{
	return (square >> 3);
}

INLINED square_t	create_square(file_t file, rank_t rank)
{
	return (file + (rank << 3));
}

INLINED square_t	opposite_square(square_t square)
{
	return (square ^ SQ_A8);
}

INLINED square_t	relative_square(square_t square, color_t color)
{
	return (square ^ (SQ_A8 * color));
}

INLINED rank_t		relative_rank(rank_t rank, color_t color)
{
	return (rank ^ (RANK_8 * color));
}

INLINED rank_t		relative_square_rank(square_t square, color_t color)
{
	return (relative_rank(rank_of_square(square), color));
}

INLINED bool		is_valid_square(square_t square)
{
	return (square >= SQ_A1 && square <= SQ_H8);
}

INLINED direction_t	pawn_direction(color_t color)
{
	return (color == WHITE ? NORTH : SOUTH);
}

#endif
