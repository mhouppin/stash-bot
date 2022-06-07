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

#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movepick.h"
#include "timeman.h"
#include "tt.h"
#include "types.h"
#include "uci.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int Reductions[64][64];
int Pruning[2][7];

void init_search_tables(void)
{
    for (int d = 1; d < 64; ++d)
        for (int m = 1; m < 64; ++m) Reductions[d][m] = -1.34 + log(d) * log(m) / 1.26;

    for (int d = 1; d < 7; ++d)
    {
        Pruning[1][d] = +3.17 + 3.66 * pow(d, 1.09);
        Pruning[0][d] = -1.25 + 3.13 * pow(d, 0.65);
    }
}

uint64_t perft(board_t *board, unsigned int depth)
{
    if (depth == 0) return (1);

    movelist_t list;
    list_all(&list, board);

    // Bulk counting: the perft number at depth 1 equals the number of legal moves.
    // Large perft speedup from not having to do the make/unmake move stuff.

    if (depth == 1) return (movelist_size(&list));

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

void update_pv(move_t *pv, move_t bestmove, move_t *subPv)
{
    size_t i;

    pv[0] = bestmove;
    for (i = 0; subPv[i] != NO_MOVE; ++i) pv[i + 1] = subPv[i];

    pv[i + 1] = NO_MOVE;
}

void main_worker_search(worker_t *worker)
{
    board_t *board = &worker->board;

    if (SearchParams.perft)
    {
        clock_t time = chess_clock();
        uint64_t nodes = perft(board, (unsigned int)SearchParams.perft);

        time = chess_clock() - time;

        uint64_t nps = nodes / (time + !time) * 1000;

        printf("info nodes %" FMT_INFO " nps %" FMT_INFO " time %" FMT_INFO "\n", (info_t)nodes,
            (info_t)nps, (info_t)time);

        return;
    }

    if (worker->rootCount == 0)
    {
        printf("info depth 0 score %s 0\n", (board->stack->checkers) ? "mate" : "cp");
        fflush(stdout);
    }
    else
    {
        // The main thread initializes all the shared things for search here:
        // node counter, time manager, workers' board and threads, and TT reset.

        tt_clear();
        timeman_init(board, &Timeman, &SearchParams, chess_clock());

        if (SearchParams.depth == 0) SearchParams.depth = MAX_PLIES;

        if (SearchParams.nodes == 0) --SearchParams.nodes;

        wpool_start_workers(&WPool);
        worker_search(worker);
    }

    // UCI protocol specifies that we shouldn't send the bestmove command
    // before the GUI sends us the "stop" in infinite mode
    // or "ponderhit" in ponder mode.

    while (!WPool.stop && (WPool.ponder || SearchParams.infinite))
        ;

    WPool.stop = true;

    if (worker->rootCount == 0)
    {
        puts("bestmove 0000");
        fflush(stdout);
        free(worker->rootMoves);
        free_boardstack(worker->stack);
        return;
    }

    // Wait for all threads to stop searching.

    wpool_wait_search_end(&WPool);

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

    if (ponderMove != NO_MOVE) printf(" ponder %s", move_to_str(ponderMove, board->chess960));

    putchar('\n');
    fflush(stdout);

    free(worker->rootMoves);
    free_boardstack(worker->stack);
}

void worker_search(worker_t *worker)
{
    board_t *board = &worker->board;

    // Reset all history related stuff.

    memset(worker->bfHistory, 0, sizeof(butterfly_history_t));
    memset(worker->ctHistory, 0, sizeof(continuation_history_t));
    memset(worker->cmHistory, 0, sizeof(countermove_history_t));
    memset(worker->capHistory, 0, sizeof(capture_history_t));
    worker->verifPlies = 0;

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
            int depth = iterDepth;
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
            search(board, depth + 1, alpha, beta, &sstack[2], true);

            // Catch search aborting

            hasSearchAborted = WPool.stop;

            sort_root_moves(
                worker->rootMoves + worker->pvLine, worker->rootMoves + worker->rootCount);
            pvScore = worker->rootMoves[worker->pvLine].score;

            // Note: we set the bound to be EXACT_BOUND when the search aborts, even if the last
            // search finished on a fail low/high.

            int bound = (abs(pvScore) == INF_SCORE) ? EXACT_BOUND
                        : (pvScore >= beta)         ? LOWER_BOUND
                        : (pvScore <= alpha)        ? UPPER_BOUND
                                                    : EXACT_BOUND;

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
                else if (multiPv > 1 && bound == EXACT_BOUND
                         && (worker->pvLine == multiPv - 1 || time > 3000))
                {
                    for (int i = 0; i < multiPv; ++i)
                        print_pv(board, worker->rootMoves + i, i + 1, iterDepth, time, bound);

                    fflush(stdout);
                }
            }

            if (hasSearchAborted) break;

            // Update aspiration window bounds in case of fail low/high

            if (bound == UPPER_BOUND)
            {
                depth = iterDepth;
                beta = (alpha + beta) / 2;
                alpha = max(-INF_SCORE, (int)pvScore - delta);
                delta += delta / 4;
                goto __retry;
            }
            else if (bound == LOWER_BOUND)
            {
                depth -= (depth > iterDepth / 2);
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

        if (hasSearchAborted) break;

        // If we went over optimal time usage, we just finished our iteration,
        // so we can safely return our bestmove.

        if (!worker->idx)
        {
            timeman_update(&Timeman, board, worker->rootMoves->move, worker->rootMoves->prevScore);
            if (timeman_can_stop_search(&Timeman, chess_clock())) break;
        }

        // If we're searching for mate and have found a mate equal or better than the given one,
        // stop the search.

        if (SearchParams.mate && worker->rootMoves->prevScore >= mate_in(SearchParams.mate * 2))
            break;

        // During fixed depth or infinite searches, allow the non-main workers to keep searching
        // as long as the main worker hasn't finished.

        if (worker->idx && iterDepth == SearchParams.depth - 1) --iterDepth;
    }

    if (worker->idx)
    {
        free(worker->rootMoves);
        free_boardstack(worker->stack);
    }
}

score_t search(
    board_t *board, int depth, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode)
{
    bool rootNode = (ss->plies == 0);
    worker_t *worker = get_worker(board);

    if (!rootNode && board->stack->rule50 >= 3 && alpha < 0 && game_has_cycle(board, ss->plies))
    {
        alpha = draw_score(worker);
        if (alpha >= beta) return (alpha);
    }

    if (depth <= 0) return (qsearch(board, alpha, beta, ss, pvNode));

    movepick_t mp;
    move_t pv[256];
    score_t bestScore = -INF_SCORE;

    if (!worker->idx) check_time();

    if (pvNode && worker->seldepth < ss->plies + 1) worker->seldepth = ss->plies + 1;

    if (WPool.stop || game_is_drawn(board, ss->plies)) return (draw_score(worker));

    if (ss->plies >= MAX_PLIES)
        return (!board->stack->checkers ? evaluate(board) : draw_score(worker));

    if (!rootNode)
    {
        // Mate pruning.

        alpha = max(alpha, mated_in(ss->plies));
        beta = min(beta, mate_in(ss->plies + 1));

        if (alpha >= beta) return (alpha);
    }

    bool inCheck = !!board->stack->checkers;
    bool improving;

    // Check for interesting TT values.

    int ttDepth = 0;
    int ttBound = NO_BOUND;
    score_t ttScore = NO_SCORE;
    move_t ttMove = NO_MOVE;
    bool found;
    hashkey_t key = board->stack->boardKey ^ ((hashkey_t)ss->excludedMove << 16);
    tt_entry_t *entry = tt_probe(key, &found);
    score_t eval;

    if (found)
    {
        ttScore = score_from_tt(entry->score, ss->plies);
        ttBound = entry->genbound & 3;
        ttDepth = entry->depth;
        ttMove = entry->bestmove;

        if (ttDepth >= depth && !pvNode)
            if (((ttBound & LOWER_BOUND) && ttScore >= beta)
                || ((ttBound & UPPER_BOUND) && ttScore <= alpha))
            {
                if ((ttBound & LOWER_BOUND) && !is_capture_or_promotion(board, ttMove))
                    update_quiet_history(board, depth, ttMove, NULL, 0, ss);

                return (ttScore);
            }
    }

    (ss + 1)->plies = ss->plies + 1;
    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

    if (inCheck)
    {
        eval = ss->staticEval = NO_SCORE;
        improving = false;
        goto __main_loop;
    }
    else if (found)
    {
        eval = ss->staticEval = entry->eval;

        if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND)) eval = ttScore;
    }
    else
    {
        eval = ss->staticEval = evaluate(board);

        // Save the eval in TT so that other workers won't have to recompute it.

        tt_save(entry, key, NO_SCORE, eval, 0, NO_BOUND, NO_MOVE);
    }

    if (rootNode && worker->pvLine) ttMove = worker->rootMoves[worker->pvLine].move;

    if (inCheck) goto __main_loop;

    // Razoring.

    if (!pvNode && depth == 1 && ss->staticEval + 150 <= alpha)
        return (qsearch(board, alpha, beta, ss, false));

    improving = ss->plies >= 2 && ss->staticEval > (ss - 2)->staticEval;

    // Futility Pruning.

    if (!pvNode && depth <= 8 && eval - 80 * (depth - improving) >= beta && eval < VICTORY)
        return (eval);

    // Null move pruning.

    if (!pvNode && depth >= 3 && ss->plies >= worker->verifPlies && !ss->excludedMove
        && eval >= beta && eval >= ss->staticEval && board->stack->material[board->sideToMove])
    {
        boardstack_t stack;

        int R = 3 + min((eval - beta) / 128, 3) + (depth / 4);

        ss->currentMove = NULL_MOVE;
        ss->pieceHistory = NULL;

        do_null_move(board, &stack);
        score_t score = -search(board, depth - R, -beta, -beta + 1, ss + 1, false);
        undo_null_move(board);

        if (score >= beta)
        {
            // Do not trust mate claims.

            if (score > MATE_FOUND) score = beta;

            // Do not trust win claims.

            if (worker->verifPlies || (depth <= 10 && abs(beta) < VICTORY)) return (score);

            // Zugzwang checking.

            worker->verifPlies = ss->plies + (depth - R) * 3 / 4;

            score_t zzscore = search(board, depth - R, beta - 1, beta, ss, false);

            worker->verifPlies = 0;

            if (zzscore >= beta) return (score);
        }
    }

    // Reduce depth if the node is absent from TT.

    if (!rootNode && !found && depth >= 5) --depth;

__main_loop:

    movepick_init(&mp, false, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;
    move_t quiets[64];
    int qcount = 0;
    move_t captures[64];
    int ccount = 0;
    bool skipQuiets = false;

    while ((currmove = movepick_next_move(&mp, skipQuiets)) != NO_MOVE)
    {
        if (rootNode)
        {
            // Exclude already searched PV lines for root nodes.

            if (find_root_move(worker->rootMoves + worker->pvLine,
                    worker->rootMoves + worker->rootCount, currmove)
                == NULL)
                continue;
        }
        else
        {
            if (!move_is_legal(board, currmove) || currmove == ss->excludedMove) continue;
        }

        moveCount++;

        bool isQuiet = !is_capture_or_promotion(board, currmove);

        if (!rootNode && bestScore > -MATE_FOUND)
        {
            // Late Move Pruning.

            if (depth <= 6 && moveCount > Pruning[improving][depth]) skipQuiets = true;

            // Futility Pruning.

            if (depth <= 4 && !inCheck && isQuiet && eval + 240 + 80 * depth <= alpha)
                skipQuiets = true;

            // SEE Pruning.

            if (depth <= 5
                && !see_greater_than(
                    board, currmove, (isQuiet ? -80 * depth : -25 * depth * depth)))
                continue;
        }

        // Report currmove info if enough time has passed.

        if (rootNode && !worker->idx && chess_clock() - Timeman.start > 3000)
        {
            printf("info depth %d currmove %s currmovenumber %d\n", depth,
                move_to_str(currmove, board->chess960), moveCount + worker->pvLine);
            fflush(stdout);
        }

        boardstack_t stack;
        score_t score = -NO_SCORE;
        int R;
        int extension = 0;
        int newDepth = depth - 1;
        bool givesCheck = move_gives_check(board, currmove);
        int histScore = isQuiet ? get_bf_history_score(
                            worker->bfHistory, piece_on(board, from_sq(currmove)), currmove)
                                : 0;

        if (!rootNode)
        {
            if (depth >= 9 && currmove == ttMove && !ss->excludedMove && (ttBound & LOWER_BOUND)
                && abs(ttScore) < VICTORY && ttDepth >= depth - 2)
            {
                score_t singularBeta = ttScore - depth;
                int singularDepth = depth / 2;

                ss->excludedMove = ttMove;
                score_t singularScore =
                    search(board, singularDepth, singularBeta - 1, singularBeta, ss, false);
                ss->excludedMove = NO_MOVE;

                if (singularScore < singularBeta)
                    extension = 1;

                else if (singularBeta >= beta)
                    return (singularBeta);
            }
            else if (givesCheck)
                extension = 1;
        }

        ss->currentMove = currmove;
        ss->pieceHistory = &worker->ctHistory[piece_on(board, from_sq(currmove))][to_sq(currmove)];

        do_move_gc(board, currmove, &stack, givesCheck);

        // Can we apply LMR ?

        if (depth >= 3 && moveCount > 2 + 2 * rootNode)
        {
            if (isQuiet)
            {
                R = Reductions[min(depth, 63)][min(moveCount, 63)];

                // Increase for non-PV nodes.

                R += !pvNode;

                // Decrease if the move is a killer or countermove.

                R -= (currmove == mp.killer1 || currmove == mp.killer2 || currmove == mp.counter);

                // Increase/decrease based on history.

                R -= histScore / 4000;

                R = clamp(R, 0, newDepth - 1);
            }
            else
                R = 1;
        }
        else
            R = 0;

        if (R) score = -search(board, newDepth - R, -alpha - 1, -alpha, ss + 1, false);

        // If LMR is not possible, or our LMR failed, do a search with no reductions.

        if ((R && score > alpha) || (!R && !(pvNode && moveCount == 1)))
            score = -search(board, newDepth + extension, -alpha - 1, -alpha, ss + 1, false);

        if (pvNode && (moveCount == 1 || score > alpha))
        {
            (ss + 1)->pv = pv;
            pv[0] = NO_MOVE;
            score = -search(board, newDepth + extension, -beta, -alpha, ss + 1, true);
        }

        undo_move(board, currmove);
        if (WPool.stop) return (0);

        if (rootNode)
        {
            root_move_t *cur = find_root_move(worker->rootMoves + worker->pvLine,
                worker->rootMoves + worker->rootCount, currmove);

            // Update PV for root.

            if (moveCount == 1 || alpha < score)
            {
                cur->score = score;
                cur->seldepth = worker->seldepth;
                cur->pv[0] = currmove;

                update_pv(cur->pv, currmove, (ss + 1)->pv);
            }
            else
                cur->score = -INF_SCORE;
        }

        if (bestScore < score)
        {
            bestScore = score;
            if (alpha < bestScore)
            {
                bestmove = currmove;
                alpha = bestScore;
                if (pvNode && !rootNode) update_pv(ss->pv, currmove, (ss + 1)->pv);

                if (alpha >= beta)
                {
                    if (isQuiet)
                        update_quiet_history(board, depth, bestmove, quiets, qcount, ss);
                    else if (moveCount != 1)
                        update_capture_history(board, depth, bestmove, captures, ccount, ss);
                    break;
                }
            }
        }

        if (qcount < 64 && isQuiet)
            quiets[qcount++] = currmove;
        else if (ccount < 64 && !isQuiet)
            captures[ccount++] = currmove;
    }

    // Checkmate/Stalemate ?

    if (moveCount == 0)
        bestScore = (ss->excludedMove) ? alpha : (board->stack->checkers) ? mated_in(ss->plies) : 0;

    if (!rootNode || worker->pvLine == 0)
    {
        int bound = (bestScore >= beta)    ? LOWER_BOUND
                    : (pvNode && bestmove) ? EXACT_BOUND
                                           : UPPER_BOUND;

        tt_save(
            entry, key, score_to_tt(bestScore, ss->plies), ss->staticEval, depth, bound, bestmove);
    }

    return (bestScore);
}

score_t qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode)
{
    worker_t *worker = get_worker(board);
    const score_t oldAlpha = alpha;
    movepick_t mp;
    move_t pv[256];

    if (!worker->idx) check_time();

    if (pvNode && worker->seldepth < ss->plies + 1) worker->seldepth = ss->plies + 1;

    if (WPool.stop || game_is_drawn(board, ss->plies)) return (draw_score(worker));

    if (ss->plies >= MAX_PLIES)
        return (!board->stack->checkers ? evaluate(board) : draw_score(worker));

    // Mate pruning.

    alpha = max(alpha, mated_in(ss->plies));
    beta = min(beta, mate_in(ss->plies + 1));

    if (alpha >= beta) return (alpha);

    // Check for interesting TT values

    score_t ttScore = NO_SCORE;
    int ttBound = NO_BOUND;
    bool found;
    tt_entry_t *entry = tt_probe(board->stack->boardKey, &found);

    if (found)
    {
        ttBound = entry->genbound & 3;
        ttScore = score_from_tt(entry->score, ss->plies);

        if (!pvNode
            && (((ttBound & LOWER_BOUND) && ttScore >= beta)
                || ((ttBound & UPPER_BOUND) && ttScore <= alpha)))
            return (ttScore);
    }

    bool inCheck = !!board->stack->checkers;
    score_t eval;
    score_t bestScore;

    if (inCheck)
    {
        eval = NO_SCORE;
        bestScore = -INF_SCORE;
    }
    else
    {
        if (found)
        {
            eval = bestScore = entry->eval;

            if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND)) eval = bestScore = ttScore;
        }
        else
            eval = bestScore = evaluate(board);

        // If not playing a capture is better because of better quiet moves,
        // allow for a simple eval return.

        alpha = max(alpha, bestScore);
        if (alpha >= beta) return (alpha);
    }

    move_t ttMove = entry->bestmove;

    (ss + 1)->plies = ss->plies + 1;

    movepick_init(&mp, true, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;

    if (pvNode) (ss + 1)->pv = pv;

    // Check if futility pruning is possible.

    const bool canFutilityPrune = (!inCheck && popcount(board->piecetypeBB[ALL_PIECES]) > 6);
    const score_t futilityBase = bestScore + 120;

    while ((currmove = movepick_next_move(&mp, false)) != NO_MOVE)
    {
        // Only analyse good capture moves.

        if (bestScore > -MATE_FOUND && mp.stage == PICK_BAD_INSTABLE) break;

        if (!move_is_legal(board, currmove)) continue;

        moveCount++;

        bool givesCheck = move_gives_check(board, currmove);

        if (bestScore > -MATE_FOUND && canFutilityPrune && !givesCheck
            && move_type(currmove) == NORMAL_MOVE)
        {
            score_t delta = futilityBase + PieceScores[ENDGAME][piece_on(board, to_sq(currmove))];

            // Check if the move is very unlikely to improve alpha.

            if (delta < alpha) continue;
        }

        ss->currentMove = currmove;
        {
            square_t to = to_sq(currmove);
            ss->pieceHistory = &worker->ctHistory[piece_on(board, to)][to];
        }

        boardstack_t stack;

        if (pvNode) pv[0] = NO_MOVE;

        do_move_gc(board, currmove, &stack, givesCheck);
        score_t score = -qsearch(board, -beta, -alpha, ss + 1, pvNode);
        undo_move(board, currmove);

        if (WPool.stop) return (0);

        if (bestScore < score)
        {
            bestScore = score;
            if (alpha < bestScore)
            {
                alpha = bestScore;
                bestmove = currmove;

                if (pvNode) update_pv(ss->pv, bestmove, (ss + 1)->pv);

                if (alpha >= beta) break;
            }
        }
    }

    if (moveCount == 0 && inCheck) bestScore = mated_in(ss->plies);

    int bound = (bestScore >= beta)       ? LOWER_BOUND
                : (bestScore <= oldAlpha) ? UPPER_BOUND
                                          : EXACT_BOUND;

    tt_save(
        entry, board->stack->boardKey, score_to_tt(bestScore, ss->plies), eval, 0, bound, bestmove);

    return (bestScore);
}
