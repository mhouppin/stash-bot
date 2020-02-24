/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   move.h                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:23:42 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 11:18:36 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef MOVE_H
# define MOVE_H

# include <stdint.h>
# include "inlining.h"
# include "piece.h"
# include "square.h"

typedef int32_t		move_t;
typedef int32_t		movetype_t;

enum
{
	NO_MOVE = 0,
	NULL_MOVE = 65,

	NORMAL_MOVE = 0,
	PROMOTION = 1 << 14,
	EN_PASSANT = 2 << 14,
	CASTLING = 3 << 14,
	MOVETYPE_MASK = 3 << 14
};

INLINED square_t	move_from_square(move_t move)
{
	return ((square_t)((move >> 6) & SQ_H8));
}

INLINED square_t	move_to_square(move_t move)
{
	return ((square_t)(move & SQ_H8));
}

INLINED int			move_squares(move_t move)
{
	return (move & 0xFFF);
}

INLINED movetype_t	type_of_move(move_t move)
{
	return (move & MOVETYPE_MASK);
}

INLINED piecetype_t	promotion_type(move_t move)
{
	return ((piecetype_t)(((move >> 12) & 3) + KNIGHT));
}

INLINED move_t		create_move(square_t from, square_t to)
{
	return ((move_t)((from << 6) + to));
}

INLINED move_t		reverse_move(move_t move)
{
	return (create_move(move_to_square(move), move_from_square(move)));
}

INLINED move_t		create_promotion(square_t from, square_t to,
					piecetype_t piecetype)
{
	return ((move_t)(PROMOTION + ((piecetype - KNIGHT) << 12)
			+ (from << 6) + to));
}

INLINED move_t		create_en_passant(square_t from, square_t to)
{
	return ((move_t)(EN_PASSANT + (from << 6) + to));
}

INLINED move_t		create_castling(square_t from, square_t to)
{
	return ((move_t)(CASTLING + (from << 6) + to));
}

INLINED bool		is_valid_move(move_t move)
{
	return (move_from_square(move) != move_to_square(move));
}

#endif
