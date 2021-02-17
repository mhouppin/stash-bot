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

#ifndef LAZY_SMP_H
# define LAZY_SMP_H

# include <pthread.h>
# include "board.h"
# include "engine.h"
# include "history.h"
# include "pawns.h"

// Struct for worker thread data.

typedef struct
{
    int idx;

    board_t         board;
    boardstack_t    *stack;
    bf_history_t    bf_history;
    ct_history_t    ct_history;
    cm_history_t    cm_history;
    pawn_entry_t    *pawn_table;

    int                 seldepth;
    int                 verif_plies;
    _Atomic uint64_t    nodes;

    root_move_t *root_moves;
    size_t      root_count;
    int         pv_line;

    pthread_t   thread;
}
worker_t;

typedef struct
{
    int         size;
    int         checks;
    worker_t    *list;
}
worker_pool_t;

extern worker_pool_t    WPool;

INLINED worker_t    *get_worker(const board_t *board)
{
    return (board->worker);
}

INLINED uint64_t    get_node_count(void)
{
    uint64_t    result = 0;

    for (int i = 0; i < WPool.size; ++i)
        result += WPool.list[i].nodes;

    return (result);
}

void    wpool_init(int threads);
void    wpool_reset(void);
void    wpool_quit(void);

#endif
