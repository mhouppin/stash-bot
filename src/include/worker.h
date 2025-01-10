/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include <pthread.h>
#include <stdatomic.h>

#include "board.h"
#include "history.h"
#include "kp_eval.h"
#include "search_params.h"
#include "timeman.h"
#include "tt.h"

// Early declaration required for the worker struct
struct _WorkerPool;

// Struct for PV lines
typedef struct _PvLine {
    Move moves[MAX_MOVES];
    u16 length;
} PvLine;

// Initializes the PV line struct
void pv_line_init(PvLine *pv_line);

// Initializes the PV line struct with the given move
void pv_line_init_move(PvLine *pv_line, Move move);

// Updates the PV line by concatenating a move and another PV line
void pv_line_update(PvLine *restrict pv_line, Move bestmove, const PvLine *restrict next);

// Struct for root moves
typedef struct _RootMove {
    Move move;
    u16 seldepth;
    Score previous_score;
    Score score;
    PvLine pv;
} RootMove;

// Initializes the root move struct
void root_move_init(RootMove *root_move, Move move);

// Locates a move in the root move array
RootMove *find_root_move(RootMove *root_moves, usize root_count, Move move);

// Sorts root moves based on their score
void sort_root_moves(RootMove *root_moves, usize root_count);

// Struct for worker thread data
typedef struct _Worker {
    Board board;
    struct _WorkerPool *pool;
    ButterflyHistory *butterfly_hist;
    ContinuationHistory *continuation_hist;
    CountermoveHistory *counter_hist;
    CaptureHistory *capture_hist;
    CorrectionHistory *correction_hist;
    KingPawnTable *king_pawn_table;

    u16 seldepth;
    u16 root_depth;
    i16 nmp_verif_plies;
    _Atomic u64 nodes;

    RootMove *root_moves;
    usize root_move_count;
    u16 pv_line;

    usize thread_index;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
    bool must_exit;
    bool is_searching;
} Worker;

// Returns the worker struct associated with the given board
INLINED Worker *board_get_worker(const Board *board) {
    assert(board->has_worker);
    return (Worker *)((uintptr_t)board - offsetof(Worker, board));
}

// Tells the board that it has a worker associated to it
INLINED void board_enable_worker(Board *board) {
    board->has_worker = true;
}

// Returns a pseudo-random draw score using the current node count
INLINED Score worker_draw_score(const Worker *worker) {
    return (Score)(atomic_load_explicit(&worker->nodes, memory_order_relaxed) & 2) - 1;
}

INLINED void worker_increment_nodes(Worker *worker) {
    atomic_fetch_add_explicit(&worker->nodes, 1, memory_order_relaxed);
}

// Initializes the worker
void worker_init(Worker *worker, usize thread_index, struct _WorkerPool *wpool);

// Frees all memory associated with the worker
void worker_destroy(Worker *worker);

// Resets the worker at the start of a new game
void worker_init_new_game(Worker *worker);

// Asks the worker to start searching
void worker_start_searching(Worker *worker);

// Helper for the worker to set up its own search state
void worker_init_search_data(Worker *worker);

// Waits for the worker to complete its search
void worker_wait_search_completion(Worker *worker);

// Entry point for the worker thread main loop
void *worker_entry_point(void *worker_ptr);

typedef struct _WorkerPool {
    pthread_attr_t worker_pthread_attr;
    usize worker_count;
    Worker **worker_list;

    Board root_board;
    SearchParams search_params;
    TranspositionTable tt;
    Timeman timeman;

    u64 check_nodes;
    atomic_bool ponder;
    atomic_bool stop;
} WorkerPool;

INLINED Worker *wpool_main_worker(WorkerPool *wpool) {
    return wpool->worker_list[0];
}

INLINED void wpool_ponderhit(WorkerPool *wpool) {
    atomic_store_explicit(&wpool->ponder, false, memory_order_relaxed);
}

INLINED bool wpool_is_pondering(const WorkerPool *wpool) {
    return atomic_load_explicit(&wpool->ponder, memory_order_relaxed);
}

INLINED void wpool_stop(WorkerPool *wpool) {
    atomic_store_explicit(&wpool->stop, true, memory_order_relaxed);
}

INLINED bool wpool_is_stopped(const WorkerPool *wpool) {
    return atomic_load_explicit(&wpool->stop, memory_order_relaxed);
}

// These functions can be called from the UCI thread.

void wpool_init(WorkerPool *wpool);
void wpool_resize(WorkerPool *wpool, usize worker_count);
void wpool_destroy(WorkerPool *wpool);
void wpool_init_new_game(WorkerPool *wpool);
void wpool_start_search(
    WorkerPool *wpool,
    const Board *root_board,
    const SearchParams *search_params
);
void wpool_wait_search_completion(WorkerPool *wpool);

// These functions can be called from the main thread.

void wpool_init_new_search(WorkerPool *wpool);
void wpool_start_aux_workers(WorkerPool *wpool);
void wpool_wait_aux_workers(WorkerPool *wpool);
void wpool_check_time(WorkerPool *wpool);

// These functions can be called from any thread.

u64 wpool_get_total_nodes(WorkerPool *wpool);

#endif
