/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "lazy_smp.h"
#include "pawns.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"

int Reductions[64][64];
int Pruning[2][7];

void init_reduction_table(void)
{
    for (int d = 1; d < 64; ++d)
        for (int m = 1; m < 64; ++m)
            Reductions[d][m] = -1.34 + log(d) * log(m) / 1.26;

    for (int d = 1; d < 7; ++d)
    {
        Pruning[1][d] = +3.17 + 3.66 * pow(d, 1.09);
        Pruning[0][d] = -1.25 + 3.13 * pow(d, 0.65);
    }
}

uint64_t perft(board_t *board, unsigned int depth)
{
    if (depth == 0)
        return (1);

    movelist_t list;
    list_all(&list, board);

    // Bulk counting: the perft number at depth 1 equals the number of legal moves.
    // Large perft speedup from not having to do the make/unmake move stuff.

    if (depth == 1)
        return (movelist_size(&list));

    uint64_t sum = 0;
    boardstack_t stack;

    for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
    {
        do_move(board, extmove->move, &stack);
        sum += perft(board, depth - 1);
        undo_move(board, extmove->move);
    }

    return (sum);
}

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

void *engine_go(void *ptr)
{
    extern goparams_t SearchParams;
    board_t *board = ptr;
    worker_t *worker = get_worker(board);

    // Code for running perft tests

    if (SearchParams.perft)
    {
        clock_t time = chess_clock();
        uint64_t nodes = perft(board, (unsigned int)SearchParams.perft);

        time = chess_clock() - time;

        uint64_t nps = nodes / (time + !time) * 1000;

        printf("info nodes %" FMT_INFO " nps %" FMT_INFO " time %" FMT_INFO "\n",
            (info_t)nodes, (info_t)nps, (info_t)time);

        return (NULL);
    }

    // If we don't have any move here, we're (stale)mated, so we can abort the search now.

    worker->rootCount = movelist_size(&SearchMoves);

    if (worker->rootCount == 0)
    {
        printf("info depth 0 score %s 0\nbestmove 0000\n", (board->stack->checkers) ? "mate" : "cp");
        fflush(stdout);
        return (NULL);
    }

    // Init root move structure here

    worker->rootMoves = malloc(sizeof(root_move_t) * worker->rootCount);

    for (size_t i = 0; i < worker->rootCount; ++i)
    {
        worker->rootMoves[i].move = SearchMoves.moves[i].move;
        worker->rootMoves[i].seldepth = 0;
        worker->rootMoves[i].prevScore = -INF_SCORE;
        worker->rootMoves[i].score = -INF_SCORE;
        worker->rootMoves[i].pv[0] = NO_MOVE;
        worker->rootMoves[i].pv[1] = NO_MOVE;
    }

    // Reset all history related stuff.

    memset(worker->bfHistory, 0, sizeof(butterfly_history_t));
    memset(worker->ctHistory, 0, sizeof(continuation_history_t));
    memset(worker->cmHistory, 0, sizeof(countermove_history_t));
    memset(worker->capHistory, 0, sizeof(capture_history_t));
    worker->verifPlies = 0;

    // The main thread initializes all the shared things for search here:
    // node counter, time manager, workers' board and threads, and TT reset.

    if (!worker->idx)
    {
        tt_clear();
        timeman_init(board, &Timeman, &SearchParams, chess_clock());

        if (SearchParams.depth == 0)
            SearchParams.depth = MAX_PLIES;

        if (SearchParams.nodes == 0)
            --SearchParams.nodes;

        WPool.checks = 1000;
        worker->nodes = 0;

        // Initialize other workers' data

        for (int i = 1; i < WPool.size; ++i)
        {
            worker_t *cur = WPool.list + i;

            cur->board = worker->board;
            cur->stack = dup_boardstack(worker->stack);
            cur->board.stack = cur->stack;
            cur->board.worker = cur;
            cur->nodes = 0;

            if (pthread_create(&cur->thread, &WorkerSettings, &engine_go, &cur->board))
            {
                perror("Unable to initialize worker thread");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Clamp MultiPV to the maximal number of lines available

    const int multiPv = min(Options.multiPv, worker->rootCount);

    for (int iterDepth = 0; iterDepth < SearchParams.depth; ++iterDepth)
    {
        bool hasSearchAborted;
        searchstack_t sstack[256];

        // Reset the search stack data

        memset(sstack, 0, sizeof(sstack));

        for (worker->pvLine = 0; worker->pvLine < multiPv; ++worker->pvLine)
        {
            // Reset the seldepth value after each depth increment, and for each
            // PV line

            worker->seldepth = 0;

            score_t alpha, beta, delta;
            score_t pvScore = worker->rootMoves[worker->pvLine].prevScore;

            // Don't set aspiration window bounds for low depths, as the scores are
            // very volatile.

            if (iterDepth <= 9)
            {
                delta = 0;
                alpha = -INF_SCORE;
                beta = INF_SCORE;
            }
            else
            {
                delta = 15;
                alpha = max(-INF_SCORE, pvScore - delta);
                beta = min(INF_SCORE, pvScore + delta);
            }

__retry:
            search(board, iterDepth + 1, alpha, beta, &sstack[2], true);

            // Catch search aborting

            hasSearchAborted = search_should_abort();

            sort_root_moves(worker->rootMoves + worker->pvLine, worker->rootMoves + worker->rootCount);
            pvScore = worker->rootMoves[worker->pvLine].score;

            // Note: we set the bound to be EXACT_BOUND when the search aborts, even if the last
            // search finished on a fail low/high.

            int bound = (abs(pvScore) == INF_SCORE) ? EXACT_BOUND
                : (pvScore >= beta) ? LOWER_BOUND
                : (pvScore <= alpha) ? UPPER_BOUND : EXACT_BOUND;

            if (bound == EXACT_BOUND)
                sort_root_moves(worker->rootMoves, worker->rootMoves + multiPv);

            if (!worker->idx)
            {
                clock_t time = chess_clock() - Timeman.start;

                // Don't update Multi-PV lines if not all analysed at current depth
                // and not enough time elapsed

                if (multiPv == 1 && (bound == EXACT_BOUND || time > 3000))
                {
                    print_pv(board, worker->rootMoves, 1, iterDepth, time, bound);
                    fflush(stdout);
                }
                else if (multiPv > 1 && bound == EXACT_BOUND && (worker->pvLine == multiPv - 1 || time > 3000))
                {
                    for (int i = 0; i < multiPv; ++i)
                        print_pv(board, worker->rootMoves + i, i + 1, iterDepth, time, bound);

                    fflush(stdout);
                }
            }

            if (hasSearchAborted)
                break ;

            // Update aspiration window bounds in case of fail low/high

            if (bound == UPPER_BOUND)
            {
                beta = (alpha + beta) / 2;
                alpha = max(-INF_SCORE, (int)pvScore - delta);
                delta += delta / 4;
                goto __retry;
            }
            else if (bound == LOWER_BOUND)
            {
                beta = min(INF_SCORE, (int)pvScore + delta);
                delta += delta / 4;
                goto __retry;
            }
        }

        // Reset root moves' score for the next search

        for (root_move_t *i = worker->rootMoves; i < worker->rootMoves + worker->rootCount; ++i)
        {
            i->prevScore = i->score;
            i->score = -INF_SCORE;
        }

        if (hasSearchAborted)
            break ;

        // If we went over optimal time usage, we just finished our iteration,
        // so we can safely return our bestmove.

        if (!worker->idx)
        {
            timeman_update(&Timeman, board, worker->rootMoves->move, worker->rootMoves->prevScore);
            if (timeman_can_stop_search(&Timeman, chess_clock()))
                break ;
        }

        // If we're searching for mate and have found a mate equal or better than the given one,
        // stop the search.

        if (SearchParams.mate && worker->rootMoves->prevScore >= mate_in(SearchParams.mate * 2))
            break ;

        // During fixed depth or infinite searches, allow the non-main workers to keep searching
        // as long as the main worker hasn't finished.

        if (worker->idx && iterDepth == SearchParams.depth - 1)
            --iterDepth;
    }

    // UCI protocol specifies that we shouldn't send the bestmove command
    // before the GUI sends us the "stop" in infinite mode
    // or "ponderhit" in ponder mode.

    if (SearchParams.infinite || SearchParams.ponder)
        while (!search_should_abort() && !(SearchParams.ponder && EnginePonderhit))
            if (!worker->idx)
                check_time();

    // The main thread sends the bestmove here and wait for all workers to stop activity

    if (!worker->idx)
    {
        printf("bestmove %s", move_to_str(worker->rootMoves->move, board->chess960));

        move_t ponderMove = worker->rootMoves->pv[1];

        // If we finished searching with a fail-high, try to see if we can get a ponder
        // move in TT.

        if (ponderMove == NO_MOVE)
        {
            boardstack_t stack;
            tt_entry_t *entry;
            bool found;

            do_move(board, worker->rootMoves->move, &stack);
            entry = tt_probe(board->stack->boardKey, &found);
            undo_move(board, worker->rootMoves->move);

            if (found)
            {
                ponderMove = entry->bestmove;

                // Take care of data races !

                if (!move_is_pseudo_legal(board, ponderMove) || !move_is_legal(board, ponderMove))
                    ponderMove = NO_MOVE;
            }
        }

        if (ponderMove != NO_MOVE)
            printf(" ponder %s", move_to_str(ponderMove, board->chess960));

        putchar('\n');
        fflush(stdout);

        if (EngineSend != DO_ABORT)
        {
            pthread_mutex_lock(&EngineMutex);
            EngineSend = DO_EXIT;
            pthread_mutex_unlock(&EngineMutex);

            for (int i = 1; i < WPool.size; ++i)
                pthread_join(WPool.list[i].thread, NULL);
        }
    }

    free(worker->rootMoves);
    free_boardstack(worker->stack);
    return (NULL);
}

void *engine_thread(void *nothing __attribute__((unused)))
{
    // Inform the UCI thread that we're ready to run

    pthread_mutex_lock(&EngineMutex);
    EngineMode = WAITING;
    pthread_cond_broadcast(&EngineCond);

    // Loop while we're not notified that a "quit" command has been issued

    while (EngineSend != DO_ABORT)
    {
        pthread_cond_wait(&EngineCond, &EngineMutex);

        if (EngineSend == DO_THINK)
        {
            EngineSend = DO_NOTHING;

            // Start by copying the board received from the "position" command

            board_t *board = &WPool.list->board;

            *board = Board;
            board->stack = dup_boardstack(Board.stack);
            WPool.list->stack = board->stack;

            // Only unlock the mutex once we're not using the global board

            pthread_mutex_unlock(&EngineMutex);
            engine_go(board);
            pthread_mutex_lock(&EngineMutex);
            EngineMode = WAITING;
            pthread_cond_broadcast(&EngineCond);
        }
    }

    pthread_mutex_unlock(&EngineMutex);

    return (NULL);
}
