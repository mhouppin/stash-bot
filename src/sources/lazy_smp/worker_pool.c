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

#include <stdio.h>
#include <stdlib.h>
#include "lazy_smp.h"

worker_pool_t   WPool;

void    wpool_init(int threads)
{
    wpool_quit();

    WPool.size = threads;
    WPool.list = malloc(sizeof(worker_t) * threads);
    if (WPool.list == NULL)
    {
        perror("Unable to allocate worker pool");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < threads; ++i)
    {
        worker_t    *worker = WPool.list + i;

        worker->idx = i;
        worker->stack = NULL;
    }

    extern board_t  g_board;

    g_board.worker = WPool.list;
}

void    wpool_quit(void)
{
    free(WPool.list);
    WPool.list = NULL;
    WPool.size = 0;
}
