/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   pop_move.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 01:28:08 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 06:42:28 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <string.h>

void	pop_move(movelist_t *mlist, size_t index)
{
	mlist->size -= 1;
	memmove(mlist->moves + index,
			mlist->moves + index + 1,
			(mlist->size - index) * sizeof(move_t));
}
