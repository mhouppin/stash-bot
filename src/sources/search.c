
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

#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "engine.h"
#include "imath.h"
#include "lazy_smp.h"
#include "movepick.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"

void update_pv(move_t *pv, move_t bestmove, move_t *subPv)
{
    size_t i;

    pv[0] = bestmove;
    for (i = 0; subPv[i] != NO_MOVE; ++i)
        pv[i + 1] = subPv[i];

    pv[i + 1] = NO_MOVE;
}

score_t search(board_t *board, int depth, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode)
{
    if (depth <= 0)
        return (qsearch(board, alpha, beta, ss));

    worker_t *worker = get_worker(board);
    movepick_t mp;
    move_t pv[256];
    score_t bestScore = -INF_SCORE;
    bool rootNode = (ss->plies == 0);

    if (!worker->idx)
        check_time();

    if (pvNode && worker->seldepth < ss->plies + 1)
        worker->seldepth = ss->plies + 1;

    if (search_should_abort() || game_is_drawn(board, ss->plies))
        return (0);

    if (ss->plies >= MAX_PLIES)
        return (!board->stack->checkers ? evaluate(board) : 0);

    // Mate pruning.

    alpha = max(alpha, mated_in(ss->plies));
    beta = min(beta, mate_in(ss->plies + 1));

    if (alpha >= beta)
        return (alpha);

    // Check for interesting tt values

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

        if (ttDepth >= depth && !pvNode)
            if (((ttBound & LOWER_BOUND) && ttScore >= beta) || ((ttBound & UPPER_BOUND) && ttScore <= alpha))
                return (ttScore);

        ttMove = entry->bestmove;
        eval = ss->staticEval = entry->eval;
        if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND))
            eval = ttScore;
    }
    else
    {
        eval = ss->staticEval = evaluate(board);

        // Save the eval in TT so that other workers won't have to recompute it

        tt_save(entry, key, NO_SCORE, eval, 0, NO_BOUND, NO_MOVE);
    }

    if (rootNode && worker->pvLine)
        ttMove = worker->rootMoves[worker->pvLine].move;

    (ss + 1)->plies = ss->plies + 1;
    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

    // Razoring.

    if (!pvNode && ss->staticEval + 150 <= alpha)
    {
        if (depth == 1)
        {
            score_t maxScore = qsearch(board, alpha, beta, ss);
            return (max(ss->staticEval + 150, maxScore));
        }
        if (ss->staticEval + 300 <= alpha && depth <= 3)
        {
            score_t maxScore = qsearch(board, alpha, beta, ss);
            if (maxScore < beta)
                return (max(ss->staticEval + 300, maxScore));
        }
    }

    bool inCheck = !!board->stack->checkers;
    bool improving = ss->plies >= 2 && ss->staticEval > (ss - 2)->staticEval;

    // Futility Pruning.

    if (!pvNode && !inCheck && depth <= 8 && eval - 80 * depth >= beta && eval < VICTORY)
        return (eval);

    // Null move pruning.

    if (!pvNode && depth >= 3 && !inCheck
        && ss->plies >= worker->verifPlies && !ss->excludedMove
        && eval >= beta && eval >= ss->staticEval
        && board->stack->material[board->sideToMove])
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

            if (score > MATE_FOUND)
                score = beta;

            // Do not trust win claims.

            if (worker->verifPlies || (depth <= 10 && abs(beta) < VICTORY))
                return (score);

            // Zugzwang checking.

            worker->verifPlies = ss->plies + (depth - R) * 3 / 4;

            score_t zzscore = search(board, depth - R, beta - 1, beta, ss, false);

            worker->verifPlies = 0;

            if (zzscore >= beta)
                return (score);
        }
    }

    movepick_init(&mp, false, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;
    move_t quiets[64];
    int qcount = 0;
    bool skipQuiets = false;

    while ((currmove = movepick_next_move(&mp, skipQuiets)) != NO_MOVE)
    {
        if (rootNode)
        {
            // Exclude already searched PV lines for root nodes

            if (find_root_move(worker->rootMoves + worker->pvLine,
                worker->rootMoves + worker->rootCount, currmove) == NULL)
                continue ;
        }
        else
        {
            if (!move_is_legal(board, currmove) || currmove == ss->excludedMove)
                continue ;
        }

        moveCount++;

        bool isQuiet = !is_capture_or_promotion(board, currmove);

        if (!rootNode && bestScore > -MATE_FOUND)
        {
            // Late Move Pruning.

            if (depth <= 5 && moveCount > depth * (improving ? 8 : 5))
                skipQuiets = true;

            // Futility Pruning.

            if (depth <= 4 && isQuiet && eval + 240 + 80 * depth <= alpha)
                skipQuiets = true;

            // SEE Pruning.

            if (depth <= 4 && !see_greater_than(board, currmove, (isQuiet ? -80 * depth : -25 * depth * depth)))
                continue ;
        }

        // Report currmove info if enough time has passed

        if (rootNode && !worker->idx && chess_clock() - Timeman.start > 3000)
        {
            printf("info depth %d currmove %s currmovenumber %d\n",
                depth, move_to_str(currmove, board->chess960), moveCount + worker->pvLine);
            fflush(stdout);
        }

        boardstack_t stack;
        score_t score = -NO_SCORE;
        int R;
        int extension = 0;
        int newDepth = depth - 1;
        bool givesCheck = move_gives_check(board, currmove);
        int histScore = isQuiet
            ? get_bf_history_score(worker->bfHistory, piece_on(board, from_sq(currmove)), currmove) : 0;

        if (!rootNode)
        {
            if (depth >= 9 && currmove == ttMove && !ss->excludedMove
                && (ttBound & LOWER_BOUND) && ttDepth >= depth - 2)
            {
                score_t singularBeta = ttScore - depth;
                int singularDepth = depth / 2;

                ss->excludedMove = ttMove;
                score_t singularScore = search(board, singularDepth, singularBeta - 1, singularBeta, ss, false);
                ss->excludedMove = NO_MOVE;

                if (singularScore < singularBeta)
                    extension = 1;
            }
            else if (givesCheck)
                extension = 1;
        }

        ss->currentMove = currmove;
        ss->pieceHistory = &worker->ctHistory[piece_on(board, from_sq(currmove))][to_sq(currmove)];

        do_move_gc(board, currmove, &stack, givesCheck);

        // Can we apply LMR ?

        if (depth >= 3 && moveCount > 2 + 2 * rootNode && isQuiet)
        {
            R = Reductions[min(depth, 63)][min(moveCount, 63)];

            // Increase for non-PV nodes

            R += !pvNode;

            // Increase/decrease based on history

            R -= histScore / 4000;

            R = max(R, 0);
        }
        else
            R = 0;

        if (R)
            score = -search(board, newDepth - R, -alpha - 1, -alpha, ss + 1, false);

        // If LMR is not possible, or our LMR failed, do a search with no reductions

        if ((R && score > alpha) || (!R && !(pvNode && moveCount == 1)))
            score = -search(board, newDepth + extension, -alpha - 1, -alpha, ss + 1, false);

        if (pvNode && (moveCount == 1 || score > alpha))
        {
            (ss + 1)->pv = pv;
            pv[0] = NO_MOVE;
            score = -search(board, newDepth + extension, -beta, -alpha, ss + 1, true);
        }

        undo_move(board, currmove);
        if (search_should_abort())
            return (0);

        if (rootNode)
        {
            root_move_t *cur = find_root_move(worker->rootMoves + worker->pvLine,
                worker->rootMoves + worker->rootCount, currmove);

            // Update PV for root

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
                if (pvNode && !rootNode)
                    update_pv(ss->pv, currmove, (ss + 1)->pv);

                if (alpha >= beta)
                {
                    if (isQuiet)
                        update_quiet_history(board, depth, bestmove, quiets, qcount, ss);
                    break ;
                }
            }
        }

        if (qcount < 64 && isQuiet)
            quiets[qcount++] = currmove;
    }

    // Checkmate/Stalemate ?

    if (moveCount == 0)
        bestScore = (ss->excludedMove) ? alpha : (board->stack->checkers) ? mated_in(ss->plies) : 0;
    
    if (!rootNode || worker->pvLine == 0)
    {
        int bound = (bestScore >= beta) ? LOWER_BOUND : (pvNode && bestmove) ? EXACT_BOUND : UPPER_BOUND;

        tt_save(entry, key, score_to_tt(bestScore, ss->plies), ss->staticEval, depth, bound, bestmove);
    }

    return (bestScore);
}

score_t qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss)
{
    worker_t *worker = get_worker(board);
    const score_t oldAlpha = alpha;
    movepick_t mp;

    if (!worker->idx)
        check_time();

    if (search_should_abort() || game_is_drawn(board, ss->plies))
        return (0);

    if (ss->plies >= MAX_PLIES)
        return (!board->stack->checkers ? evaluate(board) : 0);

    // Mate pruning.

    alpha = max(alpha, mated_in(ss->plies));
    beta = min(beta, mate_in(ss->plies + 1));

    if (alpha >= beta)
        return (alpha);

    // Check for interesting tt values

    score_t ttScore = NO_SCORE;
    int ttBound = NO_BOUND;
    bool found;
    tt_entry_t *entry = tt_probe(board->stack->boardKey, &found);

    if (found)
    {
        ttBound = entry->genbound & 3;
        ttScore = score_from_tt(entry->score, ss->plies);

        if (((ttBound & LOWER_BOUND) && ttScore >= beta) || ((ttBound & UPPER_BOUND) && ttScore <= alpha))
            return (ttScore);
    }

    score_t eval = found ? entry->eval : evaluate(board);
    score_t bestScore = -INF_SCORE;

    // If not playing a capture is better because of better quiet moves,
    // allow for a simple eval return.

    if (!board->stack->checkers)
    {
        bestScore = eval;
        if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND))
            bestScore = ttScore;

        alpha = max(alpha, bestScore);
        if (alpha >= beta)
            return (alpha);
    }

    move_t ttMove = entry->bestmove;

    (ss + 1)->plies = ss->plies + 1;

    movepick_init(&mp, true, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;

    // Check if delta pruning is possible.

    const bool deltaPruning = (!board->stack->checkers && popcount(board->piecetypeBB[ALL_PIECES]) > 6);
    const score_t deltaBase = bestScore + PAWN_EG_SCORE * 2;

    while ((currmove = movepick_next_move(&mp, false)) != NO_MOVE)
    {
        if (!move_is_legal(board, currmove))
            continue ;

        moveCount++;

        bool givesCheck = move_gives_check(board, currmove);

        if (bestScore > -MATE_FOUND && deltaPruning && !givesCheck && move_type(currmove) == NORMAL_MOVE)
        {
            score_t delta = deltaBase + PieceScores[ENDGAME][piece_on(board, to_sq(currmove))];

            // Check if the move is very unlikely to improve alpha.

            if (delta < alpha)
                continue ;
        }

        // Only analyse good capture moves.

        if (bestScore > -MATE_FOUND && !see_greater_than(board, currmove, 0))
            continue ;

        ss->currentMove = currmove;
        {
            square_t to = to_sq(currmove);
            ss->pieceHistory = &worker->ctHistory[piece_on(board, to)][to];
        }

        boardstack_t stack;

        do_move_gc(board, currmove, &stack, givesCheck);
        score_t score = -qsearch(board, -beta, -alpha, ss + 1);
        undo_move(board, currmove);

        if (search_should_abort())
            return (0);

        if (bestScore < score)
        {
            bestScore = score;
            if (alpha < bestScore)
            {
                alpha = bestScore;
                bestmove = currmove;
                if (alpha >= beta)
                    break ;
            }
        }
    }

    if (moveCount == 0 && board->stack->checkers)
        bestScore = mated_in(ss->plies);

    int bound = (bestScore >= beta) ? LOWER_BOUND : (bestScore <= oldAlpha) ? UPPER_BOUND : EXACT_BOUND;

    tt_save(entry, board->stack->boardKey, score_to_tt(bestScore, ss->plies), eval, 0, bound, bestmove);

    return (bestScore);
}