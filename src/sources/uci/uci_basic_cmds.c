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
#include "option.h"
#include "tt.h"
#include "uci.h"
#include <stdio.h>

void    uci_isready(const char *args __attribute__((unused)))
{
    puts("readyok");
    fflush(stdout);
}

void    uci_quit(const char *args __attribute__((unused)))
{
    pthread_mutex_lock(&EngineMutex);
    EngineSend = DO_ABORT;
    pthread_mutex_unlock(&EngineMutex);
    pthread_cond_signal(&EngineCond);
}

void    uci_stop(const char *args __attribute__((unused)))
{
    pthread_mutex_lock(&EngineMutex);
    EngineSend = DO_EXIT;
    pthread_mutex_unlock(&EngineMutex);
    pthread_cond_signal(&EngineCond);
}

void    uci_uci(const char *args __attribute__((unused)))
{
    puts("id name Stash " UCI_VERSION);
    puts("id author Morgan Houppin");
    show_options(&OptionList);
    puts("uciok");
    fflush(stdout);
}

void    uci_ucinewgame(const char *args)
{
    (void)args;
    wait_search_end();
    tt_bzero();
    wpool_reset();
}
