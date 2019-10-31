/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_stop.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 09:40:35 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 19:49:42 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

void	uci_stop(const char *args)
{
	(void)args;
	pthread_mutex_lock(&mtx_engine);
	g_engine_send = DO_EXIT;
	pthread_mutex_unlock(&mtx_engine);
}
