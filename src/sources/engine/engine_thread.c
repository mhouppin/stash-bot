/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lazy_smp.h"
#include "engine.h"
#include "uci.h"
#include <stdio.h>

void    *engine_thread(void *nothing __attribute__((unused)))
{
    pthread_mutex_lock(&g_engine_mutex);
    g_engine_mode = WAITING;
    pthread_cond_broadcast(&g_engine_condvar);

    while (g_engine_send != DO_ABORT)
    {
        pthread_cond_wait(&g_engine_condvar, &g_engine_mutex);

        if (g_engine_send == DO_THINK)
        {
            g_engine_send = DO_NOTHING;

            extern board_t  g_board;
            board_t         *my_board = &WPool.list->board;

            *my_board = g_board;
            my_board->stack = boardstack_dup(g_board.stack);
            WPool.list->stack = my_board->stack;

            // Only unlock the mutex once we're not using the global board

            pthread_mutex_unlock(&g_engine_mutex);

            engine_go(my_board);

            pthread_mutex_lock(&g_engine_mutex);
            g_engine_mode = WAITING;
            pthread_cond_broadcast(&g_engine_condvar);
        }
    }

    pthread_mutex_unlock(&g_engine_mutex);

    return (NULL);
}
