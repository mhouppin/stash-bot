
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

void    update_pv(move_t *pv, move_t bestmove, move_t *sub_pv)
{
    size_t  i;

    pv[0] = bestmove;
    for (i = 0; sub_pv[i] != NO_MOVE; ++i)
        pv[i + 1] = sub_pv[i];

    pv[i + 1] = NO_MOVE;
}

score_t search(board_t *board, int depth, score_t alpha, score_t beta,
        searchstack_t *ss, bool pvNode)
{
    if (depth <= 0)
        return (qsearch(board, alpha, beta, ss));

    worker_t    *worker = get_worker(board);
    movepick_t  mp;
    move_t      pv[256];
    score_t     best_value = -INF_SCORE;
    bool        root_node = (ss->plies == 0);

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

    int         tt_depth = 0;
    int         tt_bound = NO_BOUND;
    score_t     tt_score = NO_SCORE;
    move_t      ttMove = NO_MOVE;
    bool        found;
    hashkey_t   key = board->stack->boardKey ^ ((hashkey_t)ss->excludedMove << 16);
    tt_entry_t  *entry = tt_probe(key, &found);
    score_t     eval;

    if (found)
    {
        tt_score = score_from_tt(entry->score, ss->plies);
        tt_bound = entry->genbound & 3;
        tt_depth = entry->depth;

        if (tt_depth >= depth && !pvNode)
        {
            if (tt_bound == EXACT_BOUND
                || (tt_bound == LOWER_BOUND && tt_score >= beta)
                || (tt_bound == UPPER_BOUND && tt_score <= alpha))
                return (tt_score);
        }

        ttMove = entry->bestmove;
        eval = ss->static_eval = entry->eval;

        if (tt_bound & (tt_score > eval ? LOWER_BOUND : UPPER_BOUND))
            eval = tt_score;
    }
    else
    {
        eval = ss->static_eval = evaluate(board);

        // Save the eval in TT so that other workers won't have to recompute it

        tt_save(entry, key, NO_SCORE, eval, 0, NO_BOUND, NO_MOVE);
    }

    if (root_node && worker->pvLine)
        ttMove = worker->rootMoves[worker->pvLine].move;

    (ss + 1)->plies = ss->plies + 1;
    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

    // Razoring.

    if (!pvNode && ss->static_eval + 150 <= alpha)
    {
        if (depth == 1)
        {
            score_t max_score = qsearch(board, alpha, beta, ss);
            return (max(ss->static_eval + 150, max_score));
        }
        if (ss->static_eval + 300 <= alpha && depth <= 3)
        {
            score_t max_score = qsearch(board, alpha, beta, ss);
            if (max_score < beta)
                return (max(ss->static_eval + 300, max_score));
        }
    }

    bool    in_check = !!board->stack->checkers;
    bool    improving = ss->plies >= 2 && ss->static_eval > (ss - 2)->static_eval;

    // Futility Pruning.

    if (!pvNode && !in_check && depth <= 8 && eval - 80 * depth >= beta && eval < VICTORY)
        return (eval);

    // Null move pruning.

    if (!pvNode && depth >= 3 && !in_check
        && ss->plies >= worker->verifPlies && !ss->excludedMove
        && eval >= beta && eval >= ss->static_eval
        && board->stack->material[board->sideToMove])
    {
        boardstack_t    stack;

        int    nmp_reduction = 3 + min((eval - beta) / 128, 3) + (depth / 4);

        ss->currentMove = NULL_MOVE;
        ss->pieceHistory = NULL;

        do_null_move(board, &stack);

        score_t score = -search(board, depth - nmp_reduction, -beta, -beta + 1, ss + 1, false);

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

            worker->verifPlies = ss->plies + (depth - nmp_reduction) * 3 / 4;

            score_t zzscore = search(board, depth - nmp_reduction, beta - 1, beta, ss, false);

            worker->verifPlies = 0;

            if (zzscore >= beta)
                return (score);
        }
    }

    movepick_init(&mp, false, board, worker, ttMove, ss);

    move_t  currmove;
    move_t  bestmove = NO_MOVE;
    int     move_count = 0;
    move_t  quiets[64];
    int     qcount = 0;
    bool    skipQuiets = false;

    while ((currmove = movepick_next_move(&mp, skipQuiets)) != NO_MOVE)
    {
        if (root_node)
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

        move_count++;

        bool    is_quiet = !is_capture_or_promotion(board, currmove);

        if (!root_node && best_value > -MATE_FOUND)
        {
            // Late Move Pruning.

            if (depth <= 3 && move_count > depth * (improving ? 8 : 5))
                skipQuiets = true;

            // Futility Pruning.

            if (depth <= 4 && is_quiet && eval + 240 + 80 * depth <= alpha)
                skipQuiets = true;

            // SEE Pruning.

            if (depth <= 4 && !see_greater_than(board, currmove,
                (is_quiet ? -80 * depth : -25 * depth * depth)))
                continue ;
        }

        // Report currmove info if enough time has passed

        if (root_node && !worker->idx && chess_clock() - Timeman.start > 3000)
        {
            printf("info depth %d currmove %s currmovenumber %d\n",
                depth, move_to_str(currmove, board->chess960), move_count + worker->pvLine);
            fflush(stdout);
        }

        boardstack_t    stack;
        score_t         next = -NO_SCORE;
        int             reduction;
        int             extension = 0;
        int             new_depth = depth - 1;
        bool            givesCheck = move_gives_check(board, currmove);
        int             hist_score = is_quiet ? get_bf_history_score(worker->bfHistory,
            piece_on(board, from_sq(currmove)), currmove) : 0;

        if (!root_node)
        {
            if (depth >= 9 && currmove == ttMove && !ss->excludedMove
                && (tt_bound & LOWER_BOUND) && tt_depth >= depth - 2)
            {
                score_t singular_beta = tt_score - depth;
                int     singular_depth = depth / 2;

                ss->excludedMove = ttMove;
                score_t singular_score = search(board, singular_depth, singular_beta - 1,
                    singular_beta, ss, false);
                ss->excludedMove = NO_MOVE;

                if (singular_score < singular_beta)
                    extension = 1;
            }
            else if (givesCheck)
                extension = 1;
        }

        ss->currentMove = currmove;
        ss->pieceHistory = &worker->ctHistory[piece_on(board, from_sq(currmove))][to_sq(currmove)];

        do_move_gc(board, currmove, &stack, givesCheck);

        // Can we apply LMR ?
        if (depth >= 3 && move_count > 2 + 2 * root_node && is_quiet)
        {
            reduction = Reductions[min(depth, 63)][min(move_count, 63)];

            // Increase for non-PV nodes
            reduction += !pvNode;

            // Increase/decrease based on history
            reduction -= hist_score / 4000;

            reduction = max(reduction, 0);
        }
        else
            reduction = 0;

        if (reduction)
            next = -search(board, new_depth - reduction, -alpha - 1, -alpha, ss + 1, false);

        // If LMR is not possible, or our LMR failed, do a search with no reductions
        if ((reduction && next > alpha) || (!reduction && !(pvNode && move_count == 1)))
            next = -search(board, new_depth + extension, -alpha - 1, -alpha, ss + 1, false);

        if (pvNode && (move_count == 1 || next > alpha))
        {
            (ss + 1)->pv = pv;
            pv[0] = NO_MOVE;
            next = -search(board, new_depth + extension, -beta, -alpha, ss + 1, true);
        }

        undo_move(board, currmove);

        if (search_should_abort())
            return (0);

        if (root_node)
        {
            root_move_t *cur = find_root_move(worker->rootMoves + worker->pvLine,
                worker->rootMoves + worker->rootCount, currmove);

            // Update PV for root

            if (move_count == 1 || alpha < next)
            {
                cur->score = next;
                cur->seldepth = worker->seldepth;
                cur->pv[0] = currmove;

                update_pv(cur->pv, currmove, (ss + 1)->pv);
            }
            else
                cur->score = -INF_SCORE;
        }

        if (best_value < next)
        {
            best_value = next;

            if (alpha < best_value)
            {
                bestmove = currmove;
                alpha = best_value;

                if (pvNode && !root_node)
                    update_pv(ss->pv, currmove, (ss + 1)->pv);

                if (alpha >= beta)
                {
                    if (is_quiet)
                        update_quiet_history(board, depth, bestmove, quiets, qcount, ss);
                    break ;
                }
            }
        }

        if (qcount < 64 && is_quiet)
            quiets[qcount++] = currmove;
    }

    // Checkmate/Stalemate ?

    if (move_count == 0)
        best_value = (ss->excludedMove) ? alpha
            : (board->stack->checkers) ? mated_in(ss->plies) : 0;
    
    if (!root_node || worker->pvLine == 0)
    {
        int bound = (best_value >= beta) ? LOWER_BOUND
            : (pvNode && bestmove) ? EXACT_BOUND : UPPER_BOUND;

        tt_save(entry, key, score_to_tt(best_value, ss->plies), ss->static_eval,
            depth, bound, bestmove);
    }

    return (best_value);
}

score_t qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss)
{
    worker_t        *worker = get_worker(board);
    const score_t   old_alpha = alpha;
    movepick_t      mp;

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

    score_t     tt_score = NO_SCORE;
    int         tt_bound = NO_BOUND;
    bool        found;
    tt_entry_t  *entry = tt_probe(board->stack->boardKey, &found);

    if (found)
    {
        tt_bound = entry->genbound & 3;
        tt_score = score_from_tt(entry->score, ss->plies);

        if (((tt_bound & LOWER_BOUND) && tt_score >= beta)
            || ((tt_bound & UPPER_BOUND) && tt_score <= alpha))
            return (tt_score);
    }

    score_t     eval = found ? entry->eval : evaluate(board);
    score_t     best_value = -INF_SCORE;

    // If not playing a capture is better because of better quiet moves,
    // allow for a simple eval return.

    if (!board->stack->checkers)
    {
        best_value = eval;

        if (tt_bound & (tt_score > eval ? LOWER_BOUND : UPPER_BOUND))
            best_value = tt_score;

        alpha = max(alpha, best_value);
        if (alpha >= beta)
            return (alpha);
    }

    move_t  ttMove = entry->bestmove;

    (ss + 1)->plies = ss->plies + 1;

    movepick_init(&mp, true, board, worker, ttMove, ss);

    move_t  currmove;
    move_t  bestmove = NO_MOVE;
    int     move_count = 0;

    // Check if delta pruning is possible.

    const bool      delta_pruning = (!board->stack->checkers
        && popcount(board->piecetypeBB[ALL_PIECES]) > 6);
    const score_t   delta_base = best_value + PAWN_EG_SCORE * 2;

    while ((currmove = movepick_next_move(&mp, false)) != NO_MOVE)
    {
        if (!move_is_legal(board, currmove))
            continue ;

        move_count++;

        bool    givesCheck = move_gives_check(board, currmove);

        if (best_value > -MATE_FOUND && delta_pruning && !givesCheck
            && move_type(currmove) == NORMAL_MOVE)
        {
            score_t delta = delta_base + PieceScores[ENDGAME][piece_on(board, to_sq(currmove))];

            // Check if the move is very unlikely to improve alpha.

            if (delta < alpha)
                continue ;
        }

        // Only analyse good capture moves.

        if (best_value > -MATE_FOUND && !see_greater_than(board, currmove, 0))
            continue ;

        ss->currentMove = currmove;
        {
            square_t    to = to_sq(currmove);
            ss->pieceHistory = &worker->ctHistory[piece_on(board, to)][to];
        }

        boardstack_t    stack;

        do_move_gc(board, currmove, &stack, givesCheck);

        score_t next = -qsearch(board, -beta, -alpha, ss + 1);

        undo_move(board, currmove);

        if (search_should_abort())
            return (0);

        if (best_value < next)
        {
            best_value = next;
            if (alpha < best_value)
            {
                alpha = best_value;
                bestmove = currmove;
                if (alpha >= beta)
                    break ;
            }
        }
    }

    if (move_count == 0 && board->stack->checkers)
        best_value = mated_in(ss->plies);

    int bound = (best_value >= beta) ? LOWER_BOUND
        : (best_value <= old_alpha) ? UPPER_BOUND : EXACT_BOUND;

    tt_save(entry, board->stack->boardKey, score_to_tt(best_value, ss->plies),
        eval, 0, bound, bestmove);

    return (best_value);
}