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

#include "option.h"
#include "tt.h"
#include "uci.h"
#include <stdio.h>

void    uci_isready(const char *args)
{
    (void)args;
    puts("readyok");
    fflush(stdout);
}

void    uci_quit(const char *args)
{
    (void)args;
    pthread_mutex_lock(&g_engine_mutex);
    g_engine_send = DO_ABORT;
    pthread_mutex_unlock(&g_engine_mutex);
    pthread_cond_signal(&g_engine_condvar);
}

void    uci_stop(const char *args)
{
    (void)args;
    pthread_mutex_lock(&g_engine_mutex);
    g_engine_send = DO_EXIT;
    pthread_mutex_unlock(&g_engine_mutex);
    pthread_cond_signal(&g_engine_condvar);
}

void    uci_uci(const char *args)
{
    (void)args;
    puts("id name Stash " UCI_VERSION);
    puts("id author Morgan Houppin");

    show_options(&g_opthandler);

    puts("uciok");
    fflush(stdout);
}

void    uci_ucinewgame(const char *args)
{
    wait_search_end();

    (void)args;
    tt_bzero();
}
