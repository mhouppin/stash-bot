/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine_thread.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 23:30:34 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 20:24:44 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <unistd.h>

void	*engine_thread(void *nothing __attribute__((unused)))
{
	pthread_mutex_lock(&mtx_engine);

	while (g_engine_send != DO_ABORT)
	{
		pthread_cond_wait(&cv_engine, &mtx_engine);

		if (g_engine_send == DO_THINK)
		{
			g_engine_mode = THINKING;
			g_engine_send = DO_NOTHING;
			pthread_mutex_unlock(&mtx_engine);
			launch_analyse();
			pthread_mutex_lock(&mtx_engine);
			g_engine_mode = WAITING;
			g_engine_send = DO_NOTHING;
			pthread_mutex_unlock(&mtx_engine);
		}
	}

	pthread_mutex_unlock(&mtx_engine);
	pthread_cond_destroy(&cv_engine);

	return (NULL);
}
