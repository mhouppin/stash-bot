#include <stdio.h>
#include <string.h>
#include "movelist.h"
#include "uci.h"
#include "worker.h"

worker_pool_t WPool;

INLINED int rtm_greater_than(root_move_t *right, root_move_t *left)
{
    if (right->score != left->score)
        return (right->score > left->score);
    else
        return (right->prevScore > left->prevScore);
}

void sort_root_moves(root_move_t *begin, root_move_t *end)
{
    const int size = (int)(end - begin);

    for (int i = 1; i < size; ++i)
    {
        root_move_t tmp = begin[i];
        int j = i - 1;

        while (j >= 0 && rtm_greater_than(&tmp, begin + j))
        {
            begin[j + 1] = begin[j];
            --j;
        }

        begin[j + 1] = tmp;
    }
}

root_move_t *find_root_move(root_move_t *begin, root_move_t *end, move_t move)
{
    while (begin < end)
    {
        if (begin->move == move)
            return (begin);

        ++begin;
    }
    return (NULL);
}

void worker_init(worker_t *worker, size_t idx)
{
    worker->idx = idx;
    worker->stack = NULL;
    worker->pawnTable = calloc(PawnTableSize, sizeof(pawn_entry_t));
    worker->exit = false;
    worker->searching = true;

    if (worker->pawnTable == NULL)
    {
        perror("Unable to allocate pawn table");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&worker->mutex, NULL)
        || pthread_cond_init(&worker->condVar, NULL))
    {
        perror("Unable to initialize worker lock");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&worker->thread, &WorkerSettings, &worker_entry, worker))
    {
        perror("Unable to initialize worker");
        exit(EXIT_FAILURE);
    }
}

void worker_destroy(worker_t *worker)
{
    worker->exit = true;
    worker_start_search(worker);

    if (pthread_join(worker->thread, NULL))
    {
        perror("Unable to stop worker");
        exit(EXIT_FAILURE);
    }

    free(worker->pawnTable);
    pthread_mutex_destroy(&worker->mutex);
    pthread_cond_destroy(&worker->condVar);
}

void worker_reset(worker_t *worker)
{
    memset(worker->bfHistory, 0, sizeof(butterfly_history_t));
    memset(worker->ctHistory, 0, sizeof(continuation_history_t));
    memset(worker->cmHistory, 0, sizeof(countermove_history_t));
    memset(worker->capHistory, 0, sizeof(capture_history_t));
    worker->verifPlies = 0;
}

void worker_start_search(worker_t *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->searching = true;
    pthread_cond_signal(&worker->condVar);
    pthread_mutex_unlock(&worker->mutex);
}

void worker_wait_search_end(worker_t *worker)
{
    pthread_mutex_lock(&worker->mutex);
    while (worker->searching)
        pthread_cond_wait(&worker->condVar, &worker->mutex);
    pthread_mutex_unlock(&worker->mutex);
}

void *worker_entry(void *ptr)
{
    worker_t *worker = ptr;

    while (true)
    {
        pthread_mutex_lock(&worker->mutex);
        worker->searching = false;
        pthread_cond_signal(&worker->condVar);

        while (!worker->searching)
            pthread_cond_wait(&worker->condVar, &worker->mutex);

        if (worker->exit)
            break ;

        pthread_mutex_unlock(&worker->mutex);

        if (worker->idx)
            worker_search(worker);
        else
            main_worker_search(worker);
    }

    return (NULL);
}

void wpool_init(worker_pool_t *wpool, size_t threads)
{
    if (wpool->size)
    {
        worker_wait_search_end(wpool_main_worker(wpool));

        while (wpool->size)
        {
            --wpool->size;

            worker_t *curWorker = wpool->workerList[wpool->size];

            worker_destroy(curWorker);
            free(curWorker);
        }

        free(wpool->workerList);
    }

    if (threads)
    {
        wpool->workerList = malloc(sizeof(worker_t *) * threads);

        if (wpool->workerList == NULL)
        {
            perror("Unable to allocate worker pool");
            exit(EXIT_FAILURE);
        }

        while (wpool->size < threads)
        {
            wpool->workerList[wpool->size] = malloc(sizeof(worker_t));

            if (wpool->workerList[wpool->size] == NULL)
            {
                perror("Unable to allocate worker pool");
                exit(EXIT_FAILURE);
            }

            worker_init(wpool->workerList[wpool->size], wpool->size);
            wpool->size++;
        }

        wpool_reset(wpool);
    }
}

void wpool_reset(worker_pool_t *wpool)
{
    for (size_t i = 0; i < wpool->size; ++i)
        worker_reset(wpool->workerList[i]);

    wpool->checks = 0;
}

void wpool_start_search(worker_pool_t *wpool, const board_t *rootBoard,
    const goparams_t *searchParams)
{
    worker_wait_search_end(wpool_main_worker(wpool));

    wpool->stop = false;
    wpool->ponder = searchParams->ponder;

    for (size_t i = 0; i < wpool->size; ++i)
    {
        worker_t *curWorker = wpool->workerList[i];

        curWorker->nodes = 0;
        curWorker->board = *rootBoard;
        curWorker->stack = curWorker->board.stack = dup_boardstack(rootBoard->stack);
        curWorker->board.worker = curWorker;
        curWorker->rootCount = movelist_size(&SearchMoves);
        curWorker->rootMoves = malloc(sizeof(root_move_t) * curWorker->rootCount);

        if (curWorker->rootMoves == NULL && curWorker->rootCount != 0)
        {
            perror("Unable to allocate root moves");
            exit(EXIT_FAILURE);
        }

        for (size_t k = 0; k < curWorker->rootCount; ++k)
        {
            root_move_t *curRootMove = &curWorker->rootMoves[k];

            curRootMove->move = SearchMoves.moves[k].move;
            curRootMove->seldepth = 0;
            curRootMove->score = curRootMove->prevScore = -INF_SCORE;
            curRootMove->pv[0] = curRootMove->pv[1] = NO_MOVE;
        }
    }

    worker_start_search(wpool_main_worker(wpool));
}

void wpool_start_workers(worker_pool_t *wpool)
{
    for (size_t i = 1; i < wpool->size; ++i)
        worker_start_search(wpool->workerList[i]);
}

void wpool_wait_search_end(worker_pool_t *wpool)
{
    for (size_t i = 1; i < wpool->size; ++i)
        worker_wait_search_end(wpool->workerList[i]);
}

uint64_t wpool_get_total_nodes(worker_pool_t *wpool)
{
    uint64_t totalNodes = 0;

    for (size_t i = 0; i < wpool->size; ++i)
        totalNodes += wpool->workerList[i]->nodes;

    return (totalNodes);
}
