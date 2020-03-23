/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   color.h                                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 14:38:16 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 13:49:18 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef COLOR_H
# define COLOR_H

# include <stdint.h>
# include "inlining.h"

typedef int8_t	color_t;

enum
{
	WHITE,
	BLACK,
	COLOR_NB = 2
};

INLINED color_t		opposite_color(color_t color)
{
	return (color ^ BLACK);
}

#endif
