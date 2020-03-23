/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_stop.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 19:46:37 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 20:20:07 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"

void	uci_stop(const char *args)
{
	(void)args;
	pthread_mutex_lock(&g_engine_mutex);
	g_engine_send = DO_EXIT;
	pthread_mutex_unlock(&g_engine_mutex);
	pthread_cond_signal(&g_engine_condvar);
}
