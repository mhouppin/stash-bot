/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   castling.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:39:31 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/18 15:43:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
