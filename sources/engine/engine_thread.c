/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine_thread.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 20:13:25 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 20:17:18 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "uci.h"

void	*engine_thread(void *nothing __attribute__((unused)))
{
	pthread_mutex_lock(&g_engine_mutex);

	while (g_engine_send != DO_ABORT)
	{
		pthread_cond_wait(&g_engine_condvar, &g_engine_mutex);

		if (g_engine_send == DO_THINK)
		{
			g_engine_mode = THINKING;
			g_engine_send = DO_NOTHING;
			pthread_mutex_unlock(&g_engine_mutex);
			engine_go();
			pthread_mutex_lock(&g_engine_mutex);
			g_engine_mode = WAITING;
		}
	}

	pthread_mutex_unlock(&g_engine_mutex);

	return (NULL);
}
