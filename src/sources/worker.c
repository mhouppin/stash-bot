#include "worker.h"
#include "movelist.h"
#include "uci.h"
#include <stdio.h>
#include <string.h>

WorkerPool SearchWorkerPool;

INLINED int rtm_greater_than(RootMove *right, RootMove *left)
{
    if (right->score != left->score)
        return right->score > left->score;
    else
        return right->prevScore > left->prevScore;
}

void sort_root_moves(RootMove *begin, RootMove *end)
{
    const int size = (int)(end - begin);

    // We perform a simple insertion sort here.
    for (int i = 1; i < size; ++i)
    {
        RootMove tmp = begin[i];
        int j = i - 1;

        while (j >= 0 && rtm_greater_than(&tmp, begin + j))
        {
            begin[j + 1] = begin[j];
            --j;
        }

        begin[j + 1] = tmp;
    }
}

RootMove *find_root_move(RootMove *begin, RootMove *end, move_t move)
{
    while (begin < end)
    {
        if (begin->move == move) return begin;

        ++begin;
    }

    return NULL;
}

void worker_init(Worker *worker, size_t idx)
{
    worker->idx = idx;
    worker->stack = NULL;
    worker->kingPawnTable = calloc(KingPawnTableSize, sizeof(KingPawnEntry));
    worker->exit = false;
    worker->searching = true;

    if (worker->kingPawnTable == NULL)
    {
        perror("Unable to allocate pawn table");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&worker->mutex, NULL) || pthread_cond_init(&worker->condVar, NULL))
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

void worker_destroy(Worker *worker)
{
    // Notify the worker to quit its idling loop.
    worker->exit = true;
    worker_start_search(worker);

    // Wait for the worker thread to complete.
    if (pthread_join(worker->thread, NULL))
    {
        perror("Unable to stop worker");
        exit(EXIT_FAILURE);
    }

    // Destroy the pawn table and the locks initialized for the worker.
    free(worker->kingPawnTable);
    pthread_mutex_destroy(&worker->mutex);
    pthread_cond_destroy(&worker->condVar);
}

void worker_reset(Worker *worker)
{
    // Reset all history-related tables, plus the NMP disabling variable.
    memset(worker->bfHistory, 0, sizeof(butterfly_history_t));
    memset(worker->ctHistory, 0, sizeof(continuation_history_t));
    memset(worker->cmHistory, 0, sizeof(countermove_history_t));
    memset(worker->capHistory, 0, sizeof(capture_history_t));
    memset(worker->corrHistory, 0, sizeof(correction_history_t));
    memset(worker->kingPawnTable, 0, sizeof(KingPawnEntry) * KingPawnTableSize);
    worker->verifPlies = 0;
}

void worker_start_search(Worker *worker)
{
    // Notify the worker to start searching.
    pthread_mutex_lock(&worker->mutex);
    worker->searching = true;
    pthread_cond_signal(&worker->condVar);
    pthread_mutex_unlock(&worker->mutex);
}

void worker_wait_search_end(Worker *worker)
{
    // Wait for the worker to finish its search.
    pthread_mutex_lock(&worker->mutex);
    while (worker->searching) pthread_cond_wait(&worker->condVar, &worker->mutex);
    pthread_mutex_unlock(&worker->mutex);
}

void *worker_entry(void *ptr)
{
    Worker *worker = ptr;

    while (true)
    {
        // Set the worker status as non-searching, and notify all waiting
        // threads of the status change.
        pthread_mutex_lock(&worker->mutex);
        worker->searching = false;
        pthread_cond_signal(&worker->condVar);

        // Wait for a signal from the UCI thread (or main worker thread in the
        // case of SMP) to perform a task.
        while (!worker->searching) pthread_cond_wait(&worker->condVar, &worker->mutex);

        // Quit the idling loop if asked.
        if (worker->exit) break;

        pthread_mutex_unlock(&worker->mutex);

        // In case of SMP search, the main worker thread will have to do
        // additional work for launching other workers.
        if (worker->idx)
            worker_search(worker);
        else
            main_worker_search(worker);
    }

    return NULL;
}

void wpool_init(WorkerPool *wpool, size_t threads)
{
    if (wpool->size)
    {
        // Wait for the current search to complete if needed.
        worker_wait_search_end(wpool_main_worker(wpool));

        // Destroy the current worker list.
        while (wpool->size)
        {
            --wpool->size;

            Worker *curWorker = wpool->workerList[wpool->size];

            worker_destroy(curWorker);
            free(curWorker);
        }

        free(wpool->workerList);
    }

    // Don't do anything if the thread count is zero. This is done so that we
    // can destroy the worker pool after the UCI loop in main() by calling this
    // function with threads=0.
    if (threads)
    {
        wpool->workerList = malloc(sizeof(Worker *) * threads);

        if (wpool->workerList == NULL)
        {
            perror("Unable to allocate worker pool");
            exit(EXIT_FAILURE);
        }

        while (wpool->size < threads)
        {
            // Perform an independent allocation for each worker data block.
            wpool->workerList[wpool->size] = malloc(sizeof(Worker));

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

void wpool_new_search(WorkerPool *wpool)
{
    // Reset the verification ply counter used in NMP for each thread.
    for (size_t i = 0; i < wpool->size; ++i) wpool->workerList[i]->verifPlies = 0;

    // Reset the periodical time checking counter as well.
    wpool->checks = 1;
}

void wpool_reset(WorkerPool *wpool)
{
    // Reset the history tables and the verification ply counter used in NMP for
    // each thread.
    for (size_t i = 0; i < wpool->size; ++i) worker_reset(wpool->workerList[i]);

    // Reset the periodical time checking counter as well.
    wpool->checks = 1;
}

void wpool_start_search(WorkerPool *wpool, const Board *rootBoard, const SearchParams *searchParams)
{
    // Wait for the current search to complete if needed.
    worker_wait_search_end(wpool_main_worker(wpool));

    // Reset the stop flag, and set the ponder flag if indicated by the "go"
    // command.
    atomic_store_explicit(&wpool->stop, false, memory_order_relaxed);
    atomic_store_explicit(&wpool->ponder, searchParams->ponder, memory_order_relaxed);

    for (size_t i = 0; i < wpool->size; ++i)
    {
        Worker *curWorker = wpool->workerList[i];

        // Reset the node counter for each worker, and configure the position to
        // search.
        atomic_store_explicit(&curWorker->nodes, 0, memory_order_relaxed);
        curWorker->board = *rootBoard;
        curWorker->stack = curWorker->board.stack = dup_boardstack(rootBoard->stack);
        curWorker->board.worker = curWorker;
        curWorker->rootCount = movelist_size(&UciSearchMoves);
        curWorker->rootMoves = malloc(sizeof(RootMove) * curWorker->rootCount);

        if (curWorker->rootMoves == NULL && curWorker->rootCount != 0)
        {
            perror("Unable to allocate root moves");
            exit(EXIT_FAILURE);
        }

        // Set each root move to a default value.
        for (size_t k = 0; k < curWorker->rootCount; ++k)
        {
            RootMove *curRootMove = &curWorker->rootMoves[k];

            curRootMove->move = UciSearchMoves.moves[k].move;
            curRootMove->seldepth = 0;
            curRootMove->score = curRootMove->prevScore = -INF_SCORE;
            curRootMove->pv[0] = curRootMove->pv[1] = NO_MOVE;
        }
    }

    // Wake up the main worker that will do the rest of the job for us.
    worker_start_search(wpool_main_worker(wpool));
}

void wpool_start_workers(WorkerPool *wpool)
{
    for (size_t i = 1; i < wpool->size; ++i) worker_start_search(wpool->workerList[i]);
}

void wpool_wait_search_end(WorkerPool *wpool)
{
    for (size_t i = 1; i < wpool->size; ++i) worker_wait_search_end(wpool->workerList[i]);
}

uint64_t wpool_get_total_nodes(WorkerPool *wpool)
{
    uint64_t totalNodes = 0;

    // Compute the sum of the current node counts across all workers.
    for (size_t i = 0; i < wpool->size; ++i)
        totalNodes += atomic_load_explicit(&wpool->workerList[i]->nodes, memory_order_relaxed);

    return totalNodes;
}
