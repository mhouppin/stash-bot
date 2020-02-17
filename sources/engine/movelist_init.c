/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   movelist_init.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 22:11:05 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/17 08:15:02 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <stdlib.h>

movelist_t	*movelist_init(void)
{
	movelist_t	*m;

	m = (movelist_t *)malloc(sizeof(movelist_t));
	if (m == NULL)
		return (NULL);
	m->moves = (move_t *)malloc(sizeof(move_t) * 256);
	if (m->moves == NULL)
	{
		free(m);
		return (NULL);
	}
	m->values = (move_t *)malloc(sizeof(value_t) * 256);
	if (m->values == NULL)
	{
		free(m->moves);
		free(m);
		return (NULL);
	}
	m->size = 0;
	return (m);
}
