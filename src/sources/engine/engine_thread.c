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
    pthread_mutex_lock(&EngineMutex);
    EngineMode = WAITING;
    pthread_cond_broadcast(&EngineCond);

    while (EngineSend != DO_ABORT)
    {
        pthread_cond_wait(&EngineCond, &EngineMutex);

        if (EngineSend == DO_THINK)
        {
            EngineSend = DO_NOTHING;

            board_t         *my_board = &WPool.list->board;

            *my_board = Board;
            my_board->stack = dup_boardstack(Board.stack);
            WPool.list->stack = my_board->stack;

            // Only unlock the mutex once we're not using the global board

            pthread_mutex_unlock(&EngineMutex);

            engine_go(my_board);

            pthread_mutex_lock(&EngineMutex);
            EngineMode = WAITING;
            pthread_cond_broadcast(&EngineCond);
        }
    }

    pthread_mutex_unlock(&EngineMutex);

    return (NULL);
}
