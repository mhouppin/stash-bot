/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   movelist_quit.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 22:12:52 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 22:17:38 by mhouppin    ###    #+. /#+    ###.fr     */
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
		free(mlist);
	}
}
