#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include "board.h"
#include "history.h"
#include "pawns.h"
#include "uci.h"

// Struct for search params.

typedef struct goparams_s
{
    clock_t wtime;
    clock_t btime;
    clock_t winc;
    clock_t binc;
    int movestogo;
    int depth;
    size_t nodes;
    int mate;
    int infinite;
    int perft;
    int ponder;
    clock_t movetime;
}
goparams_t;

extern goparams_t SearchParams;

// Struct for root moves.

typedef struct root_move_s
{
    move_t  move;
    int seldepth;
    score_t prevScore;
    score_t score;
    move_t  pv[512];
}
root_move_t;

void sort_root_moves(root_move_t *begin, root_move_t *end);
root_move_t *find_root_move(root_move_t *begin, root_move_t *end, move_t move);
void print_pv(const board_t *board, root_move_t *rootMove, int multiPv,
    int depth, clock_t time, int bound);

// Struct for worker thread data.

typedef struct worker_s
{
    board_t board;
    boardstack_t *stack;
    butterfly_history_t bfHistory;
    continuation_history_t ctHistory;
    countermove_history_t cmHistory;
    capture_history_t capHistory;
    pawn_entry_t *pawnTable;

    int seldepth;
    int verifPlies;
    _Atomic uint64_t nodes;

    root_move_t *rootMoves;
    size_t rootCount;
    int pvLine;

    size_t idx;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condVar;
    bool exit;
    bool searching;
}
worker_t;

INLINED worker_t *get_worker(const board_t *board)
{
    return (board->worker);
}

INLINED score_t draw_score(const worker_t *worker)
{
    return (worker->nodes & 2) - 1;
}

void worker_init(worker_t *worker, size_t idx);
void worker_destroy(worker_t *worker);
void worker_search(worker_t *worker);
void main_worker_search(worker_t *worker);
void worker_reset(worker_t *worker);
void worker_start_search(worker_t *worker);
void worker_wait_search_end(worker_t *worker);
void *worker_entry(void *worker);

typedef struct worker_pool_s
{
    size_t size;
    int checks;

    _Atomic bool ponder;
    _Atomic bool stop;

    worker_t **workerList;
}
worker_pool_t;

extern worker_pool_t WPool;

INLINED worker_t *wpool_main_worker(worker_pool_t *wpool)
{
    return wpool->workerList[0];
}

void wpool_init(worker_pool_t *wpool, size_t threads);
void wpool_reset(worker_pool_t *wpool);
void wpool_start_search(worker_pool_t *wpool, const board_t *rootBoard,
    const goparams_t *searchParams);
void wpool_start_workers(worker_pool_t *wpool);
void wpool_wait_search_end(worker_pool_t *wpool);
uint64_t wpool_get_total_nodes(worker_pool_t *wpool);

#endif