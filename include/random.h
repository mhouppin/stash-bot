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
	return (((uint64_t)rand() << 51)
		^ ((uint64_t)rand() << 38)
		^ ((uint64_t)rand() << 25)
		^ ((uint64_t)rand() << 12)
		^ (uint64_t)rand());
}

#endif
