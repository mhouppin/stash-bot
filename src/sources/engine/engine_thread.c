/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

			extern board_t	g_board;
			board_t			dup = g_board;

			dup.stack = boardstack_dup(g_board.stack);

			engine_go(&dup);

			boardstack_free(dup.stack);

			pthread_mutex_lock(&g_engine_mutex);
			g_engine_mode = WAITING;
			pthread_cond_broadcast(&g_engine_condvar);
		}
	}

	pthread_mutex_unlock(&g_engine_mutex);

	return (NULL);
}
