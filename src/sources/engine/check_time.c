/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

#include "engine.h"
#include "lazy_smp.h"
#include "uci.h"

void    check_time(void)
{
    extern goparams_t   g_goparams;

    if (--WPool.checks > 0)
        return ;

    // Reset check counter

    WPool.checks = 1000;

    // If we are in infinite mode, or the stop has already been set,
    // we can safely return.

    if (g_goparams.infinite || g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
        return ;

    if (get_node_count() >= g_goparams.nodes)
        goto __set_stop;

    if (g_goparams.movetime || g_goparams.wtime || g_goparams.winc)
    {
        clock_t end = g_goparams.start + g_goparams.maximal_time;
        if (chess_clock() > end)
            goto __set_stop;
    }

    return ;

__set_stop:
    pthread_mutex_lock(&g_engine_mutex);
    g_engine_send = DO_EXIT;
    pthread_mutex_unlock(&g_engine_mutex);
}
