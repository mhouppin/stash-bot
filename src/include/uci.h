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

#ifndef UCI_H
# define UCI_H

# include <pthread.h>
# include <stdbool.h>
# include <stddef.h>
# include <time.h>
# include "inlining.h"

# define UCI_VERSION    "v26.4"

enum    e_egn_mode
{
    WAITING,
    THINKING
};

enum    e_egn_send
{
    DO_NOTHING,
    DO_THINK,
    DO_EXIT,
    DO_ABORT
};

typedef struct  goparams_s
{
    clock_t     wtime;
    clock_t     btime;
    clock_t     winc;
    clock_t     binc;
    int         movestogo;
    int         depth;
    size_t      nodes;
    int         mate;
    int         infinite;
    int         perft;
    clock_t     movetime;
}               goparams_t;

typedef struct  ucioptions_s
{
    long        threads;
    long        hash;
    long        move_overhead;
    long        multi_pv;
    bool        chess960;
}               ucioptions_t;

extern pthread_attr_t   g_engine_attr;
extern pthread_mutex_t  g_engine_mutex;
extern pthread_cond_t   g_engine_condvar;
extern enum e_egn_mode  g_engine_mode;
extern enum e_egn_send  g_engine_send;
extern goparams_t       g_goparams;

typedef struct  cmdlink_s
{
    const char  *cmd_name;
    void        (*call)(const char *);
}               cmdlink_t;

void    wait_search_end(void);
char    *get_next_token(char **str);

void    uci_bench(const char *args);
void    uci_d(const char *args);
void    uci_debug(const char *args);
void    uci_go(const char *args);
void    uci_isready(const char *args);
void    uci_position(const char *args);
void    uci_quit(const char *args);
void    uci_setoption(const char *args);
void    uci_stop(const char *args);
void    uci_uci(const char *args);
void    uci_ucinewgame(const char *args);

#endif
