/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   random.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 16:40:36 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/21 16:42:08 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef RANDOM_H
# define RANDOM_H

# include <stdint.h>
# include <stdlib.h>

uint64_t	qrandom(void)
{
	return (((uint64_t)random() << 43)
		^ ((uint64_t)random() << 21)
		^ (uint64_t)random());
}

#endif
