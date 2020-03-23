/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   random.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 16:40:36 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 12:06:38 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef RANDOM_H
# define RANDOM_H

# include <stdint.h>
# include "inlining.h"

extern uint64_t		g_seed;

INLINED void		qseed(uint64_t value)
{
	g_seed = value;
}

INLINED uint64_t	qrandom(void)
{
	g_seed ^= g_seed << 13;
	g_seed ^= g_seed >> 7;
	g_seed ^= g_seed << 17;

	return (g_seed);
}

#endif
