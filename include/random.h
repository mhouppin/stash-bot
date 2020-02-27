/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   random.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 16:40:36 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 16:48:23 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef RANDOM_H
# define RANDOM_H

# include <stdint.h>

uint64_t	qrandom(void)
{
	static uint64_t	seed = 1048592;

	seed ^= seed << 13;
	seed ^= seed >> 7;
	seed ^= seed << 17;

	return (seed);
}

#endif
