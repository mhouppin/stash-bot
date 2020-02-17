/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   movelist_quit.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 22:12:52 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/17 08:15:16 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <stdlib.h>

void	movelist_quit(movelist_t *mlist)
{
	if (mlist)
	{
		free(mlist->moves);
		free(mlist->values);
		free(mlist);
	}
}
