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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "pawns.h"
#include "timeman.h"
#include "tt.h"

int         Reductions[64][64];

void        init_reduction_table(void)
{
    for (int d = 1; d < 64; ++d)
        for (int m = 1; m < 64; ++m)
            Reductions[d][m] = -1.34 + log(d) * log(m) / 1.26;
}

uint64_t    perft(board_t *board, unsigned int depth)
{
    if (depth == 0)
        return (1);
    else
    {
        movelist_t  list;
        list_all(&list, board);

        // Bulk counting: the perft number at depth 1 equals the number of legal moves.
        // Large perft speedup from not having to do the make/unmake move stuff.

        if (depth == 1)
            return (movelist_size(&list));

        uint64_t        sum = 0;
        boardstack_t    stack;

        for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
        {
            do_move(board, extmove->move, &stack);
            sum += perft(board, depth - 1);
            undo_move(board, extmove->move);
        }
        return (sum);
    }
}

void        *engine_go(void *ptr)
{
    extern goparams_t   SearchParams;
    board_t             *board = ptr;
    worker_t            *worker = get_worker(board);
    searchstack_t       sstack[256];

    // Code for running perft tests

    if (SearchParams.perft)
    {
        clock_t     time = chess_clock();
        uint64_t    nodes = perft(board, (unsigned int)SearchParams.perft);

        time = chess_clock() - time;

        uint64_t    nps = nodes / (time + !time) * 1000;

        printf("info nodes %" FMT_INFO " nps %" FMT_INFO " time %" FMT_INFO "\n",
            (info_t)nodes, (info_t)nps, (info_t)time);

        return (NULL);
    }

    worker->root_count = movelist_size(&SearchMoves);
    // If we don't have any move here, we're (stale)mated, so we can abort the search now.
    if (worker->root_count == 0)
    {
        printf("info depth 0 score %s 0\nbestmove 0000\n", (board->stack->checkers) ? "mate" : "cp");
        fflush(stdout);
        return (NULL);
    }

    // Init root move structure here

    worker->root_moves = malloc(sizeof(root_move_t) * worker->root_count);
    for (size_t i = 0; i < worker->root_count; ++i)
    {
        worker->root_moves[i].move = SearchMoves.moves[i].move;
        worker->root_moves[i].seldepth = 0;
        worker->root_moves[i].previous_score = -INF_SCORE;
        worker->root_moves[i].score = -INF_SCORE;
        worker->root_moves[i].pv[0] = NO_MOVE;
    }

    // Reset all history related stuff, and zero-fill the searck stack.

    memset(sstack, 0, sizeof(sstack));
    memset(worker->bf_history, 0, sizeof(bf_history_t));
    memset(worker->ct_history, 0, sizeof(ct_history_t));
    memset(worker->cm_history, 0, sizeof(cm_history_t));
    worker->verif_plies = 0;

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
            worker_t    *cur = WPool.list + i;

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

    const int   multi_pv = min(Options.multi_pv, worker->root_count);

    for (int iter_depth = 0; iter_depth < SearchParams.depth; ++iter_depth)
    {
        bool            has_search_aborted;

        for (worker->pv_line = 0; worker->pv_line < multi_pv; ++worker->pv_line)
        {
            worker->seldepth = 0;

            score_t _alpha, _beta, _delta;
            score_t pv_score = worker->root_moves[worker->pv_line].previous_score;

            // Don't set aspiration window bounds for low depths, as the scores are
            // very volatile

            if (iter_depth <= 9)
            {
                _delta = 0;
                _alpha = -INF_SCORE;
                _beta = INF_SCORE;
            }
            else
            {
                _delta = 15;
                _alpha = max(-INF_SCORE, pv_score - _delta);
                _beta = min(INF_SCORE, pv_score + _delta);
            }

__retry:
            search(board, iter_depth + 1, _alpha, _beta, &sstack[1], true);

            // Catch search aborting

            has_search_aborted = search_should_abort();

            sort_root_moves(worker->root_moves + worker->pv_line,
                    worker->root_moves + worker->root_count);
            pv_score = worker->root_moves[worker->pv_line].score;

            int     bound = (abs(pv_score) == INF_SCORE) ? EXACT_BOUND
                : (pv_score >= _beta) ? LOWER_BOUND
                : (pv_score <= _alpha) ? UPPER_BOUND : EXACT_BOUND;

            if (bound == EXACT_BOUND)
                sort_root_moves(worker->root_moves, worker->root_moves + multi_pv);

            if (!worker->idx)
            {
                clock_t time = chess_clock() - Timeman.start;

                // Don't update Multi-PV lines if not all analysed at current depth
                // and not enough time elapsed

                if (multi_pv == 1 && (bound == EXACT_BOUND || time > 3000))
                {
                    print_pv(board, worker->root_moves, 1, iter_depth, time, bound);
                    fflush(stdout);
                }
                else if (multi_pv > 1 && bound == EXACT_BOUND && (worker->pv_line == multi_pv - 1  || time > 3000))
                {
                    for (int i = 0; i < multi_pv; ++i)
                        print_pv(board, worker->root_moves + i, i + 1, iter_depth, time, bound);

                    fflush(stdout);
                }
            }

            if (has_search_aborted)
                break ;

            // Update aspiration window bounds in case of fail low/high

            if (bound == UPPER_BOUND)
            {
                _beta = (_alpha + _beta) / 2;
                _alpha = max(-INF_SCORE, (int)pv_score - _delta);
                _delta += _delta / 4;
                goto __retry;
            }
            else if (bound == LOWER_BOUND)
            {
                _beta = min(INF_SCORE, (int)pv_score + _delta);
                _delta += _delta / 4;
                goto __retry;
            }
        }

        for (root_move_t *i = worker->root_moves; i < worker->root_moves + worker->root_count; ++i)
        {
            i->previous_score = i->score;
            i->score = -INF_SCORE;
        }

        if (has_search_aborted)
            break ;

        // If we went over optimal time usage, we just finished our iteration,
        // so we can safely return our bestmove.

        if (!worker->idx)
        {
            timeman_update(&Timeman, board, worker->root_moves->move, worker->root_moves->previous_score);
            if (timeman_can_stop_search(&Timeman, chess_clock()))
                break ;
        }

        if (SearchParams.mate && worker->root_moves->previous_score >= mate_in(SearchParams.mate * 2))
            break ;
    }

    // UCI protocol specifies that we shouldn't send the bestmove command
    // before the GUI sends us the "stop" in infinite mode.

    if (SearchParams.infinite)
        while (!search_should_abort())
            if (!worker->idx)
                check_time();

    if (!worker->idx)
    {
        printf("bestmove %s\n", move_to_str(worker->root_moves->move, board->chess960));
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

    free(worker->root_moves);
    free_boardstack(worker->stack);
    return (NULL);
}
