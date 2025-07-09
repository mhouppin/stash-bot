/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#include "worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"
#include "wmalloc.h"

void pv_line_init(PvLine *pv_line) {
    pv_line->length = 0;
}

void pv_line_init_move(PvLine *pv_line, Move move) {
    pv_line->length = 1;
    pv_line->moves[0] = move;
}

void pv_line_update(PvLine *restrict pv_line, Move bestmove, const PvLine *restrict next) {
    pv_line->moves[0] = bestmove;
    pv_line->length = next->length + 1;

    for (u16 i = 0; i < next->length; ++i) {
        pv_line->moves[i + 1] = next->moves[i];
    }
}

void root_move_init(RootMove *root_move, Move move) {
    root_move->move = move;
    root_move->seldepth = 0;
    root_move->previous_score = -INF_SCORE;
    root_move->score = -INF_SCORE;
    pv_line_init_move(&root_move->pv, move);
}

RootMove *find_root_move(RootMove *root_moves, usize root_count, Move move) {
    for (usize i = 0; i < root_count; ++i) {
        if (root_moves[i].move == move) {
            return &root_moves[i];
        }
    }

    return NULL;
}

static i32 compare_root_moves(const RootMove *lhs, const RootMove *rhs) {
    if (lhs->score != rhs->score) {
        return (i32)lhs->score - (i32)rhs->score;
    }

    return (i32)lhs->previous_score - (i32)rhs->previous_score;
}

void sort_root_moves(RootMove *root_moves, usize root_count) {
    // We perform a simple insertion sort here.
    for (usize i = 1; i < root_count; ++i) {
        RootMove tmp = root_moves[i];

        isize j = (isize)i - 1;

        while (j >= 0 && compare_root_moves(&tmp, &root_moves[j]) > 0) {
            root_moves[j + 1] = root_moves[j];
            --j;
        }

        root_moves[j + 1] = tmp;
    }
}

void worker_init(Worker *worker, usize thread_index, struct WorkerPool *pool) {
    worker->thread_index = thread_index;
    worker->butterfly_hist = wrap_aligned_alloc(64, sizeof(ButterflyHistory));
    worker->continuation_hist = wrap_aligned_alloc(64, sizeof(ContinuationHistory));
    worker->counter_hist = wrap_aligned_alloc(64, sizeof(CountermoveHistory));
    worker->capture_hist = wrap_aligned_alloc(64, sizeof(CaptureHistory));
    worker->pawn_corrhist = wrap_aligned_alloc(64, sizeof(CorrectionHistory));
    worker->nonpawn_corrhist = wrap_aligned_alloc(64, sizeof(CorrectionHistory) * COLOR_NB);
    worker->minor_corrhist = wrap_aligned_alloc(64, sizeof(CorrectionHistory));
    worker->king_pawn_table = wrap_aligned_alloc(64, sizeof(KingPawnTable));
    worker->root_moves = wrap_malloc(sizeof(RootMove) * MAX_MOVES);
    worker->must_exit = false;
    worker->is_searching = true;
    worker->pool = pool;

    if (pthread_mutex_init(&worker->mutex, NULL) || pthread_cond_init(&worker->condvar, NULL)) {
        perror("Unable to initialize worker lock");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(
            &worker->thread,
            &worker->pool->worker_pthread_attr,
            worker_entry_point,
            worker
        )) {
        perror("Unable to initialize worker thread");
        exit(EXIT_FAILURE);
    }

    atomic_init(&worker->nodes, 0);
    worker_init_new_game(worker);
}

void worker_destroy(Worker *worker) {
    pthread_mutex_lock(&worker->mutex);
    worker->must_exit = true;
    pthread_mutex_unlock(&worker->mutex);
    worker_start_searching(worker);

    if (pthread_join(worker->thread, NULL)) {
        perror("Unable to stop worker");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&worker->mutex);
    pthread_cond_destroy(&worker->condvar);
    wrap_aligned_free(worker->butterfly_hist);
    wrap_aligned_free(worker->continuation_hist);
    wrap_aligned_free(worker->counter_hist);
    wrap_aligned_free(worker->capture_hist);
    wrap_aligned_free(worker->pawn_corrhist);
    wrap_aligned_free(worker->nonpawn_corrhist);
    wrap_aligned_free(worker->minor_corrhist);
    wrap_aligned_free(worker->king_pawn_table);
    free(worker->root_moves);
}

void worker_init_new_game(Worker *worker) {
    memset(worker->butterfly_hist, 0, sizeof(ButterflyHistory));
    memset(worker->continuation_hist, 0, sizeof(ContinuationHistory));
    memset(worker->counter_hist, 0, sizeof(CountermoveHistory));
    memset(worker->capture_hist, 0, sizeof(CaptureHistory));
    memset(worker->pawn_corrhist, 0, sizeof(CorrectionHistory));
    memset(worker->nonpawn_corrhist, 0, sizeof(CorrectionHistory) * COLOR_NB);
    memset(worker->minor_corrhist, 0, sizeof(CorrectionHistory));
    memset(worker->king_pawn_table, 0, sizeof(KingPawnTable));
}

void worker_start_searching(Worker *worker) {
    pthread_mutex_lock(&worker->mutex);
    worker->is_searching = true;
    pthread_cond_signal(&worker->condvar);
    pthread_mutex_unlock(&worker->mutex);
}

void worker_init_search_data(Worker *worker) {
    const Movelist *searchmoves = &worker->pool->search_params.searchmoves;

    board_clone(&worker->board, &worker->pool->root_board);
    board_enable_worker(&worker->board);

    worker->seldepth = 0;
    worker->root_depth = 0;
    worker->nmp_verif_plies = 0;
    worker->root_move_count = movelist_size(searchmoves);

    for (usize i = 0; i < worker->root_move_count; ++i) {
        root_move_init(&worker->root_moves[i], searchmoves->moves[i].move);
    }

    worker->pv_line = 0;
}

void worker_wait_search_completion(Worker *worker) {
    pthread_mutex_lock(&worker->mutex);

    while (worker->is_searching) {
        pthread_cond_wait(&worker->condvar, &worker->mutex);
    }

    pthread_mutex_unlock(&worker->mutex);
}

void *worker_entry_point(void *worker_ptr) {
    Worker *worker = (Worker *)worker_ptr;

    while (true) {
        // Set the worker status as non-searching, and notify all waiting threads of the status
        // change.
        pthread_mutex_lock(&worker->mutex);
        worker->is_searching = false;
        pthread_cond_signal(&worker->condvar);

        // Wait for a signal from the UCI thread (or the main worker thread in the case of SMP) to
        // perform a task.
        while (!worker->is_searching) {
            pthread_cond_wait(&worker->condvar, &worker->mutex);
        }

        if (worker->must_exit) {
            pthread_mutex_unlock(&worker->mutex);
            break;
        }

        pthread_mutex_unlock(&worker->mutex);

        if (worker->thread_index == 0) {
            main_worker_search(worker);
        } else {
            worker_search(worker);
        }
    }

    return NULL;
}

void wpool_init(WorkerPool *wpool) {
    if (pthread_attr_init(&wpool->worker_pthread_attr)
        || pthread_attr_setstacksize(&wpool->worker_pthread_attr, (usize)4 * 1024 * 1024)) {
        perror("Unable to initialize worker thread attributes");
        exit(EXIT_FAILURE);
    }

    wpool->worker_count = 0;
    wpool->worker_list = NULL;
    tt_init(&wpool->tt);
    tt_resize(&wpool->tt, 16, 1);
    memset(&wpool->root_board, 0, sizeof(Board));
    wpool->check_nodes = 0;
    atomic_init(&wpool->ponder, false);
    atomic_init(&wpool->stop, false);
    wpool_resize(wpool, 1);
}

void wpool_resize(WorkerPool *wpool, usize worker_count) {
    if (wpool->worker_count != 0) {
        wpool_wait_search_completion(wpool);
    }

    if (wpool->worker_count >= worker_count) {
        while (wpool->worker_count > worker_count) {
            --wpool->worker_count;

            Worker *cur_worker = wpool->worker_list[wpool->worker_count];

            worker_destroy(cur_worker);
            wrap_aligned_free(cur_worker);
        }

        return;
    }

    wpool->worker_list = wrap_realloc(wpool->worker_list, sizeof(Worker *) * worker_count);

    while (wpool->worker_count < worker_count) {
        // Perform an independent allocation for each worker data block.
        // Force alignment on 64 bytes to avoid false sharing issues in search.
        wpool->worker_list[wpool->worker_count] =
            wrap_aligned_alloc(64, usize_next_multiple_of(sizeof(Worker), 64));

        worker_init(wpool->worker_list[wpool->worker_count], wpool->worker_count, wpool);
        ++wpool->worker_count;
    }

    wpool_init_new_game(wpool);
}

void wpool_destroy(WorkerPool *wpool) {
    wpool_wait_search_completion(wpool);

    while (wpool->worker_count) {
        --wpool->worker_count;

        Worker *cur_worker = wpool->worker_list[wpool->worker_count];

        worker_destroy(cur_worker);
        wrap_aligned_free(cur_worker);
    }

    free(wpool->worker_list);
    boardstack_destroy(wpool->root_board.stack);
    pthread_attr_destroy(&wpool->worker_pthread_attr);
    tt_destroy(&wpool->tt);
}

void wpool_init_new_game(WorkerPool *wpool) {
    for (usize i = 0; i < wpool->worker_count; ++i) {
        worker_init_new_game(wpool->worker_list[i]);
    }

    tt_init_new_game(&wpool->tt, wpool->worker_count);
}

void wpool_start_search(
    WorkerPool *wpool,
    const Board *root_board,
    const SearchParams *search_params
) {
    wpool_wait_search_completion(wpool);

    atomic_store_explicit(&wpool->stop, false, memory_order_relaxed);
    atomic_store_explicit(&wpool->ponder, search_params->ponder, memory_order_relaxed);

    // Init the time manager here to account for the potential worker wakeup/init delay.
    timeman_init(&wpool->timeman, root_board, search_params, timepoint_now());

    boardstack_destroy(wpool->root_board.stack);
    board_clone(&wpool->root_board, root_board);
    search_params_copy(&wpool->search_params, search_params);
    worker_start_searching(wpool_main_worker(wpool));
}

void wpool_wait_search_completion(WorkerPool *wpool) {
    // The main thread is the last thread to complete search, so we can just wait for it.
    worker_wait_search_completion(wpool->worker_list[0]);
}

void wpool_init_new_search(WorkerPool *wpool) {
    wpool->check_nodes = 1;
    tt_new_search(&wpool->tt);

    for (usize i = 0; i < wpool->worker_count; ++i) {
        atomic_store_explicit(&wpool->worker_list[i]->nodes, 0, memory_order_relaxed);
    }
}

void wpool_start_aux_workers(WorkerPool *wpool) {
    for (usize i = 1; i < wpool->worker_count; ++i) {
        worker_start_searching(wpool->worker_list[i]);
    }
}

void wpool_wait_aux_workers(WorkerPool *wpool) {
    for (usize i = 1; i < wpool->worker_count; ++i) {
        worker_wait_search_completion(wpool->worker_list[i]);
    }
}

void wpool_check_time(WorkerPool *wpool) {
    if (--wpool->check_nodes > 0) {
        return;
    }

    wpool->check_nodes = wpool->timeman.delay_check_nodes;

    if (wpool->search_params.infinite || wpool_is_stopped(wpool)) {
        return;
    }

    if (wpool_get_total_nodes(wpool) >= wpool->search_params.nodes
        || timeman_must_stop_search(&wpool->timeman, wpool, timepoint_now())) {
        wpool_stop(wpool);
    }
}

u64 wpool_get_total_nodes(WorkerPool *wpool) {
    u64 total = 0;

    for (usize i = 0; i < wpool->worker_count; ++i) {
        total += atomic_load_explicit(&wpool->worker_list[i]->nodes, memory_order_relaxed);
    }

    return total;
}
