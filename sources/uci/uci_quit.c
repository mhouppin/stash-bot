/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_quit.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 09:38:45 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 09:52:06 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

void	uci_quit(const char *args)
{
	(void)args;
	pthread_mutex_lock(&mtx_engine);
	g_engine_send = DO_ABORT;
	pthread_mutex_unlock(&mtx_engine);
}
