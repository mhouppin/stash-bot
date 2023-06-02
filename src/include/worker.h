/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#ifndef WORKER_H
#define WORKER_H

#include "board.h"
#include "history.h"
#include "pawns.h"
#include "uci.h"
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

// Struct for search params.

typedef struct _SearchParams
{
    clock_t wtime;
    clock_t btime;
    clock_t winc;
    clock_t binc;
    int movestogo;
    int depth;
    uint64_t nodes;
    int mate;
    int infinite;
    int perft;
    int ponder;
    clock_t movetime;
} SearchParams;

extern SearchParams UciSearchParams;

// Struct for root moves.

typedef struct _RootMove
{
    move_t move;
    int seldepth;
    score_t prevScore;
    score_t score;
    move_t pv[512];
} RootMove;

void sort_root_moves(RootMove *begin, RootMove *end);
RootMove *find_root_move(RootMove *begin, RootMove *end, move_t move);
void print_pv(
    const Board *board, RootMove *rootMove, int multiPv, int depth, clock_t time, int bound);

// Struct for worker thread data.

typedef struct worker_s
{
    Board board;
    Boardstack *stack;
    butterfly_history_t bfHistory;
    continuation_history_t ctHistory;
    countermove_history_t cmHistory;
    capture_history_t capHistory;
    PawnEntry *pawnTable;

    int seldepth;
    int verifPlies;
    _Atomic uint64_t nodes;

    RootMove *rootMoves;
    size_t rootCount;
    int pvLine;

    size_t idx;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condVar;
    bool exit;
    bool searching;
} worker_t;

INLINED worker_t *get_worker(const Board *board) { return board->worker; }

INLINED score_t draw_score(const worker_t *worker)
{
    return (atomic_load_explicit(&worker->nodes, memory_order_relaxed) & 2) - 1;
}

void worker_init(worker_t *worker, size_t idx);
void worker_destroy(worker_t *worker);
void worker_search(worker_t *worker);
void main_worker_search(worker_t *worker);
void worker_reset(worker_t *worker);
void worker_start_search(worker_t *worker);
void worker_wait_search_end(worker_t *worker);
void *worker_entry(void *worker);

typedef struct _WorkerPool
{
    size_t size;
    int checks;

    _Atomic bool ponder;
    _Atomic bool stop;

    worker_t **workerList;
} WorkerPool;

extern WorkerPool SearchWorkerPool;

INLINED worker_t *wpool_main_worker(WorkerPool *wpool) { return wpool->workerList[0]; }

void wpool_init(WorkerPool *wpool, size_t threads);
void wpool_new_search(WorkerPool *wpool);
void wpool_reset(WorkerPool *wpool);
void wpool_start_search(
    WorkerPool *wpool, const Board *rootBoard, const SearchParams *searchParams);
void wpool_start_workers(WorkerPool *wpool);
void wpool_wait_search_end(WorkerPool *wpool);
uint64_t wpool_get_total_nodes(WorkerPool *wpool);

#endif
