/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   piece.h                                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 14:42:11 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 14:03:30 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
