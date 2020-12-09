/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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
#include <stdlib.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

score_t search(board_t *board, int depth, score_t alpha, score_t beta,
        searchstack_t *ss, bool pv_node)
{
    if (depth <= 0)
        return (qsearch(board, alpha, beta, ss));

    worker_t    *worker = get_worker(board);
    movelist_t  list;
    move_t      pv[256];
    score_t     best_value = -INF_SCORE;

    if (!worker->idx)
        check_time();

    if (pv_node && worker->seldepth < ss->plies + 1)
        worker->seldepth = ss->plies + 1;

    if (g_engine_send == DO_EXIT || g_engine_send == DO_ABORT
        || is_draw(board, ss->plies))
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
    move_t      tt_move = NO_MOVE;
    bool        found;
    hashkey_t   key = board->stack->board_key ^ ((hashkey_t)ss->excluded_move << 16);
    tt_entry_t  *entry = tt_probe(key, &found);
    score_t     eval;

    if (found)
    {
        tt_score = score_from_tt(entry->score, ss->plies);
        tt_bound = entry->genbound & 3;
        tt_depth = entry->depth;

        if (tt_depth >= depth && !pv_node)
        {
            if (tt_bound == EXACT_BOUND
                || (tt_bound == LOWER_BOUND && tt_score >= beta)
                || (tt_bound == UPPER_BOUND && tt_score <= alpha))
                return (tt_score);
        }

        tt_move = entry->bestmove;
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

    (ss + 1)->plies = ss->plies + 1;
    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

    // Razoring.

    if (!pv_node && ss->static_eval + Razor_LightMargin < beta)
    {
        if (depth == 1)
        {
            score_t max_score = qsearch(board, alpha, beta, ss);
            return (max(ss->static_eval + Razor_LightMargin, max_score));
        }
        if (ss->static_eval + Razor_HeavyMargin < beta && depth <= 3)
        {
            score_t max_score = qsearch(board, alpha, beta, ss);
            if (max_score < beta)
                return (max(ss->static_eval + Razor_HeavyMargin, max_score));
        }
    }

    bool    in_check = !!board->stack->checkers;

    // Futility Pruning.

    if (!pv_node && !in_check && depth <= 8
        && eval - 80 * depth >= beta && eval < VICTORY)
        return (eval);

    // Null move pruning.

    if (!pv_node && depth >= NMP_MinDepth && !in_check
        && ss->plies >= worker->verif_plies && !ss->excluded_move
        && eval >= beta && eval >= ss->static_eval)
    {
        boardstack_t    stack;

        int    nmp_reduction = NMP_BaseReduction
            + min((eval - beta) / NMP_EvalScale, NMP_MaxEvalReduction)
            + (depth / 4);

        do_null_move(board, &stack);

        score_t score = -search(board, depth - nmp_reduction, -beta, -beta + 1,
                ss + 1, false);

        undo_null_move(board);

        if (score >= beta)
        {
            // Do not trust mate claims.

            if (score > MATE_FOUND)
                score = beta;

            // Do not trust win claims.

            if (worker->verif_plies || (depth <= NMP_TrustDepth && abs(beta) < VICTORY))
                return (score);

            // Zugzwang checking.

            worker->verif_plies = ss->plies + (depth - nmp_reduction) * 3 / 4;

            score_t zzscore = search(board, depth - nmp_reduction, beta - 1, beta,
                ss, false);

            worker->verif_plies = 0;

            if (zzscore >= beta)
                return (score);
        }
    }

    if (depth > 7 && !tt_move)
        --depth;

    list_pseudo(&list, board);
    generate_move_values(&list, board, tt_move, ss->killers);

    move_t  bestmove = NO_MOVE;
    int     move_count = 0;
    move_t  quiets[64];
    int     qcount = 0;

    for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
    {
        place_top_move(extmove, list.last);
        const move_t    currmove = extmove->move;

        if (!board_legal(board, currmove) || currmove == ss->excluded_move)
            continue ;

        move_count++;

        boardstack_t    stack;
        score_t         next;
        int             reduction;
        int             extension = 0;
        int             new_depth = depth - 1;
        bool            is_quiet = !is_capture_or_promotion(board, currmove);
        bool            gives_check = move_gives_check(board, currmove);

        if (depth >= 9 && currmove == tt_move && !ss->excluded_move
            && abs(tt_score) < VICTORY && (tt_bound & LOWER_BOUND)
            && tt_depth >= depth - 2)
        {
            score_t singular_beta = tt_score - depth * 2;
            int     singular_depth = depth / 2;

            ss->excluded_move = tt_move;
            score_t singular_score = search(board, singular_depth, singular_beta - 1,
                singular_beta, ss, false);
            ss->excluded_move = NO_MOVE;

            if (singular_score < singular_beta)
                extension = 1;
        }
        else if (gives_check)
            extension = 1;

        do_move_gc(board, currmove, &stack, gives_check);

        // Can we apply LMR ?
        if (depth >= LMR_MinDepth && move_count > LMR_MinMoves && !board->stack->checkers)
            reduction = max(0, Reductions[min(depth, 63)][min(move_count, 63)]);
        else
            reduction = 0;

        if (reduction)
            next = -search(board, new_depth - reduction, -alpha - 1, -alpha, ss + 1, false);

        // If LMR is not possible, or our LMR failed, do a search with no reductions
        if ((reduction && next > alpha) || (!reduction && !(pv_node && move_count == 1)))
            next = -search(board, new_depth + extension, -alpha - 1, -alpha, ss + 1, false);

        if (pv_node && (move_count == 1 || next > alpha))
        {
            (ss + 1)->pv = pv;
            pv[0] = NO_MOVE;
            next = -search(board, new_depth + extension, -beta, -alpha, ss + 1, true);
        }

        undo_move(board, currmove);

        if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
            return (0);

        if (best_value < next)
        {
            best_value = next;

            if (alpha < best_value)
            {
                bestmove = currmove;
                alpha = best_value;

                if (pv_node)
                {
                    ss->pv[0] = bestmove = currmove;

                    size_t  j;
                    for (j = 0; (ss + 1)->pv[j] != NO_MOVE; ++j)
                        ss->pv[j + 1] = (ss + 1)->pv[j];

                    ss->pv[j + 1] = NO_MOVE;
                }

                if (alpha >= beta)
                {
                    if (is_quiet)
                        update_quiet_history(worker->history, board, depth,
                            bestmove, quiets, qcount, ss);
                    break ;
                }
            }
        }

        if (qcount < 64 && is_quiet)
            quiets[qcount++] = currmove;

        if (depth < 4 && best_value > -MATE_FOUND && qcount > depth * 8)
            break ;
    }

    // Checkmate/Stalemate ?

    if (move_count == 0)
        best_value = (ss->excluded_move) ? alpha
            : (board->stack->checkers) ? mated_in(ss->plies) : 0;

    // Do not erase entries with higher depth for same position.

    if (entry->key != key || entry->depth <= depth)
    {
        int bound = (best_value >= beta) ? LOWER_BOUND
            : (pv_node && bestmove) ? EXACT_BOUND : UPPER_BOUND;

        tt_save(entry, key, score_to_tt(best_value, ss->plies),
            ss->static_eval, depth, bound, bestmove);
    }

    return (best_value);
}
