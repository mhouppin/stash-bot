/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "evaluate.h"
#include "movepicker.h"
#include "psq_table.h"
#include "syncio.h"
#include "wdl.h"

static i16 Reductions[2][MAX_MOVES];

void search_init(void) {
    Reductions[0][0] = Reductions[1][0] = 0;

    for (usize i = 1; i < 256; ++i) {
        Reductions[0][i] = (i16)(log(i) * 10.60 + 4.10); // Noisy LMR formula
        Reductions[1][i] = (i16)(log(i) * 19.70 + 11.14); // Quiet LMR formula
    }
}

static i16 lmr_base_value(i16 depth, i16 move_count, bool improving, bool is_quiet) {
    return (-400 + Reductions[is_quiet][depth] * Reductions[is_quiet][move_count] + !improving * 504
           )
        / 1024;
}

static i16 lmp_threshold(i16 depth, bool improving) {
    const i16 result = improving ? 70 + 7 * depth * depth : 13 + 4 * depth * depth;

    return result / 16;
}

static i32 get_conthist_move_score(const Board *board, const Searchstack *ss, Move move) {
    const Piece moved_piece = board_piece_on(board, move_from(move));
    const Square to = move_to(move);
    i32 history = 0;

    if ((ss - 1)->piece_history != NULL) {
        history += piece_hist_score((ss - 1)->piece_history, moved_piece, to);
    }

    if ((ss - 2)->piece_history != NULL) {
        history += piece_hist_score((ss - 2)->piece_history, moved_piece, to);
    }

    if ((ss - 4)->piece_history != NULL) {
        history += piece_hist_score((ss - 4)->piece_history, moved_piece, to);
    }

    return history;
}

static i32 get_move_history_score(
    const Board *board,
    const Worker *worker,
    const Searchstack *ss,
    Move move
) {
    const Piece moved_piece = board_piece_on(board, move_from(move));

    return butterfly_hist_score(worker->butterfly_hist, moved_piece, move)
        + get_conthist_move_score(board, ss, move);
}

static u64 perft(Board *board, u16 depth) {
    Movelist list;
    Boardstack stack;
    u64 total;

    if (depth == 0) {
        return 1;
    }

    movelist_generate_legal(&list, board);

    if (depth == 1) {
        return movelist_size(&list);
    }

    total = 0;

    for (const ExtendedMove *extmove = movelist_begin(&list); extmove < movelist_end(&list);
         ++extmove) {
        board_do_move(board, extmove->move, &stack);
        total += perft(board, depth - 1);
        board_undo_move(board, extmove->move);
    }

    return total;
}

static void info_append_score(String *info_str, Score score, bool normalize) {
    if (normalize) {
        score = normalized_score(score);
    }

    if (score_is_mate(score)) {
        string_push_back_strview(info_str, STATIC_STRVIEW("mate "));
        string_push_back_i64(info_str, (score > 0 ? MATE - score + 1 : -MATE - score) / 2);
    } else {
        string_push_back_strview(info_str, STATIC_STRVIEW("cp "));
        string_push_back_i64(info_str, score);
    }
}

static void print_pv(
    Worker *worker,
    const Board *board,
    const RootMove *root_move,
    u16 pv_line,
    u16 depth,
    Duration elapsed,
    Bound bound
) {
    static StringView BoundStr[4] = {
        STATIC_STRVIEW(""),
        STATIC_STRVIEW(" upperbound"),
        STATIC_STRVIEW(" lowerbound"),
        STATIC_STRVIEW(""),
    };
    const u64 nodes = wpool_get_total_nodes(worker->pool);
    const u64 nps = compute_nps(nodes, elapsed);
    const bool searched = (root_move->score != -INF_SCORE);
    const Score root_score = searched ? root_move->score : root_move->previous_score;

    String info_str;

    string_init(&info_str);
    string_reserve(&info_str, 2048);
    string_push_back_strview(&info_str, STATIC_STRVIEW("info depth "));
    string_push_back_u64(&info_str, u16_max(depth - !searched, 1));
    string_push_back_strview(&info_str, STATIC_STRVIEW(" seldepth "));
    string_push_back_u64(&info_str, root_move->seldepth);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" multipv "));
    string_push_back_u64(&info_str, pv_line);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" score "));
    info_append_score(&info_str, root_score, worker->pool->search_params.normalize_score);
    string_push_back_strview(&info_str, BoundStr[bound]);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" nodes "));
    string_push_back_u64(&info_str, nodes);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" nps "));
    string_push_back_u64(&info_str, nps);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" hashfull "));
    string_push_back_u64(&info_str, tt_hashfull(&worker->pool->tt));
    string_push_back_strview(&info_str, STATIC_STRVIEW(" time "));
    string_push_back_i64(&info_str, elapsed);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" pv"));

    for (usize i = 0; i < root_move->pv.length; ++i) {
        string_push_back(&info_str, ' ');
        string_push_back_strview(&info_str, board_move_to_uci(board, root_move->pv.moves[i]));
    }

    string_push_back(&info_str, '\n');
    fwrite_string(stdout, &info_str);
    fflush(stdout);
    string_destroy(&info_str);
}

static void print_currmove(const Board *board, i16 depth, Move currmove, i16 movenumber) {
    String info_str;

    string_init(&info_str);
    string_reserve(&info_str, 64);
    string_push_back_strview(&info_str, STATIC_STRVIEW("info depth "));
    string_push_back_i64(&info_str, depth);
    string_push_back_strview(&info_str, STATIC_STRVIEW(" currmove "));
    string_push_back_strview(&info_str, board_move_to_uci(board, currmove));
    string_push_back_strview(&info_str, STATIC_STRVIEW(" currmovenumber "));
    string_push_back_i64(&info_str, movenumber);
    string_push_back(&info_str, '\n');

    fwrite_string(stdout, &info_str);
    fflush(stdout);
    string_destroy(&info_str);
}

void searchstack_init(Searchstack *ss) {
    memset(ss, 0, sizeof(Searchstack) * 256);

    for (i16 i = 0; i < 256; ++i) {
        (ss + i)->plies = i - 4;
    }
}

void main_worker_search(Worker *worker) {
    Board *board = &worker->board;
    SearchParams *search_params = &worker->pool->search_params;

    worker_init_search_data(worker);

    if (search_params->perft != 0) {
        const Timepoint start = timepoint_now();
        const u64 nodes = perft(board, search_params->perft);
        const Duration elapsed = timepoint_diff(start, timepoint_now());

        printf(
            "info nodes " FORMAT_LARGE_INT " nps " FORMAT_LARGE_INT " time " FORMAT_LARGE_INT "\n",
            (LargeInt)nodes,
            (LargeInt)compute_nps(nodes, elapsed),
            (LargeInt)elapsed
        );
        goto cleanup;
    }

    // Stop the search here if there exists no legal moves due to checkmate/stalemate.
    if (movelist_size(&search_params->searchmoves) == 0) {
        printf("info depth 1 score %s 0\n", board->stack->checkers ? "mate" : "cp");
        fflush(stdout);
    } else {
        wpool_init_new_search(worker->pool);

        if (search_params->depth == 0) {
            search_params->depth = MAX_PLIES;
        }

        if (search_params->nodes == 0) {
            search_params->nodes = UINT64_MAX;
        }

        wpool_start_aux_workers(worker->pool);
        worker_search(worker);
    }

    // The UCI protocol specifies that we shouldn't send the `bestmove` command before the GUI sends
    // us the `stop` in infinite mode or `ponderhit` in ponder mode.
    while (!wpool_is_stopped(worker->pool)
           && (wpool_is_pondering(worker->pool) || search_params->infinite))
        ;

    wpool_stop(worker->pool);

    // We don't need to wait for auxiliary threads when we have no root moves since we never wake
    // them up.
    if (movelist_size(&search_params->searchmoves) == 0) {
        sync_lock_stdout();
        puts("bestmove 0000");
        fflush(stdout);
        sync_unlock_stdout();
        goto cleanup;
    }

    wpool_wait_aux_workers(worker->pool);

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("bestmove "));
    fwrite_strview(stdout, board_move_to_uci(board, worker->root_moves->move));

    Move ponder_move = NO_MOVE;

    // If we finished searching with a fail-high, try to see if we can get a ponder move from the
    // TT.
    if (worker->root_moves->pv.length == 1) {
        Boardstack stack;
        TranspositionEntry *tt_entry;
        bool found;

        board_do_move(board, worker->root_moves->move, &stack);
        tt_entry = tt_probe(&worker->pool->tt, board->stack->board_key, &found);
        board_undo_move(board, worker->root_moves->move);

        if (found) {
            ponder_move = tt_entry->bestmove;

            // Careful with data races !
            if (!board_move_is_pseudolegal(board, ponder_move)
                || !board_move_is_legal(board, ponder_move)) {
                ponder_move = NO_MOVE;
            }
        }
    } else {
        ponder_move = worker->root_moves->pv.moves[1];
    }

    if (ponder_move != NO_MOVE) {
        fwrite_strview(stdout, STATIC_STRVIEW(" ponder "));
        fwrite_strview(stdout, board_move_to_uci(board, ponder_move));
    }

    fputc('\n', stdout);
    fflush(stdout);
    sync_unlock_stdout();

cleanup:
    boardstack_destroy(board->stack);
}

void worker_search(Worker *worker) {
    const SearchParams *search_params = &worker->pool->search_params;

    // Be careful to not initialize the main worker twice to avoid memory leaks
    if (worker->thread_index != 0) {
        worker_init_search_data(worker);
    }

    // Clamp MultiPV to the maximal number of available root moves.
    const u16 multi_pv = (u16)u64_min((u64)search_params->multi_pv, (u64)worker->root_move_count);
    Searchstack sstack[256];

    searchstack_init(sstack);

    for (worker->root_depth = 1; worker->root_depth <= search_params->depth; ++worker->root_depth) {
        for (worker->pv_line = 0; worker->pv_line < multi_pv; ++worker->pv_line) {
            if (!worker_search_pv_line(worker, worker->root_depth, multi_pv, sstack)) {
                break;
            }
        }

        // Reset root moves' score for the next search.
        for (usize i = 0; i < worker->root_move_count; ++i) {
            worker->root_moves[i].previous_score = worker->root_moves[i].score;
            worker->root_moves[i].score = -INF_SCORE;
        }

        if (wpool_is_stopped(worker->pool)) {
            break;
        }

        if (worker->thread_index == 0) {
            timeman_update(
                &worker->pool->timeman,
                &worker->board,
                worker->root_moves->move,
                worker->root_moves->previous_score
            );

            // If we went over optimal time usage, we just finished our iteration, so we can safely
            // stop search.
            if (timeman_can_stop_search(&worker->pool->timeman, worker->pool, timepoint_now())) {
                break;
            }
        }

        // Stop the search if we found a mate equal or better than what was requested.
        if (search_params->mate != 0
            && worker->root_moves->previous_score >= mate_in(search_params->mate * 2)) {
            break;
        }

        // Allow the non-main workers to keep searching as long as the main worker hasn't requested
        // a stop.
        if (worker->thread_index != 0 && worker->root_depth == search_params->depth) {
            --worker->root_depth;
        }
    }

    // Be careful to not destroy the main worker's board here since we still need it.
    if (worker->thread_index != 0) {
        boardstack_destroy(worker->board.stack);
    }
}

bool worker_search_pv_line(Worker *worker, u16 depth, u16 multi_pv, Searchstack *ss) {
    Score alpha, beta, delta;
    Score pv_score = worker->root_moves[worker->pv_line].previous_score;
    Bound bound = NO_BOUND;

    worker->seldepth = 0;
    // Don't set aspiration window bounds for low depths, as the scores are very volatile.
    if (worker->root_depth <= 8) {
        delta = 0;
        alpha = -INF_SCORE;
        beta = INF_SCORE;
    } else {
        delta = 8 + (Score)(i16_abs(pv_score) / 81);
        alpha = (Score)i32_max(-INF_SCORE, (i32)pv_score - delta);
        beta = (Score)i32_min(INF_SCORE, (i32)pv_score + delta);
    }

    do {
        search(true, &worker->board, (i16)depth, alpha, beta, ss + 4, false);
        sort_root_moves(
            worker->root_moves + worker->pv_line,
            worker->root_move_count - worker->pv_line
        );
        pv_score = worker->root_moves[worker->pv_line].score;

        // Note: we set the bound to be EXACT_BOUND when the search aborts, even if the last search
        // finished on a fail low/high (this also allows us to exit the search loop without checking
        // for a search stop again).
        bound = wpool_is_stopped(worker->pool) ? EXACT_BOUND
            : (pv_score >= beta)               ? LOWER_BOUND
            : (pv_score <= alpha)              ? UPPER_BOUND
                                               : EXACT_BOUND;

        if (bound == EXACT_BOUND) {
            sort_root_moves(worker->root_moves, (usize)multi_pv);
        }

        if (worker->thread_index == 0) {
            Duration elapsed = timepoint_diff(worker->pool->timeman.start, timepoint_now());
            const bool late_info = elapsed > 3000;
            const bool single_pv = multi_pv == 1;
            const bool iter_done = worker->pv_line + 1 == multi_pv;

            // Don't update Multi-PV lines if they are not all analysed at current search depth and
            // not enough time has passed to avoid flooding the standard output.
            if ((late_info && single_pv) || (bound == EXACT_BOUND && (late_info || iter_done))) {
                for (u16 i = 0; i < multi_pv; ++i) {
                    print_pv(
                        worker,
                        &worker->board,
                        &worker->root_moves[i],
                        i + 1,
                        worker->root_depth,
                        elapsed,
                        bound
                    );
                }

                fflush(stdout);
            }
        }

        // Update aspiration window bounds in case of fail low/high.
        if (bound == UPPER_BOUND) {
            depth = worker->root_depth;
            beta = (Score)(((i32)alpha + beta) / 2);
            alpha = (Score)i32_max(-INF_SCORE, (i32)pv_score - delta);
        } else if (bound == LOWER_BOUND) {
            depth -= (depth - 1 > (worker->root_depth - 1) / 2);
            beta = (Score)i32_min(INF_SCORE, (i32)pv_score + delta);
        }

        delta += (i32)delta * 76 / 256;
    } while (bound != EXACT_BOUND);

    return !wpool_is_stopped(worker->pool);
}

Score search(
    bool pv_node,
    Board *board,
    i16 depth,
    Score alpha,
    Score beta,
    Searchstack *ss,
    bool cut_node
) {
    const bool root_node = ss->plies == 0;
    Worker *worker = board_get_worker(board);

    // Perform an early check for repetition detections.
    if (!root_node && board->stack->rule50 >= 3 && alpha < 0
        && board_game_contains_cycle(board, ss->plies)) {
        alpha = worker_draw_score(worker);

        if (alpha >= beta) {
            return alpha;
        }
    }

    // Drop into qsearch if the depth isn't strictly positive.
    if (depth <= 0) {
        return qsearch(pv_node, board, alpha, beta, ss);
    }

    Movepicker mp;
    Score best_score = -INF_SCORE;

    // Verify the time usage if we're the main thread.
    if (worker->thread_index == 0) {
        wpool_check_time(worker->pool);
    }

    // Update the seldepth value if needed.
    if (pv_node) {
        worker->seldepth = u16_max(worker->seldepth, ss->plies + 1);
    }

    // Stop the search if the game is drawn or the timeman/UCI thread asks for a stop.
    if (!root_node && (wpool_is_stopped(worker->pool) || board_game_is_drawn(board, ss->plies))) {
        return worker_draw_score(worker);
    }

    // Stop the search after MAX_PLIES recursive search calls.
    if (ss->plies >= MAX_PLIES) {
        return !board->stack->checkers ? evaluate(board) : worker_draw_score(worker);
    }

    // Mate distance pruning
    if (!root_node) {
        alpha = i16_max(alpha, mated_in(ss->plies));
        beta = i16_min(beta, mate_in(ss->plies + 1));

        if (alpha >= beta) {
            return alpha;
        }
    }

    TranspositionEntry *tt_entry;
    Score tt_score = NO_SCORE;
    Move tt_move = NO_MOVE;
    i16 tt_depth = 0;
    Bound tt_bound = NO_BOUND;
    bool tt_noisy = false;
    bool tt_found;
    Key key = board->stack->board_key ^ ((Key)ss->excluded_move << 16);
    Score raw_eval;
    Score eval;

    // Probe the TT for information on the current position.
    tt_entry = tt_probe(&worker->pool->tt, key, &tt_found);

    if (tt_found) {
        tt_score = score_from_tt(tt_entry->score, ss->plies);
        tt_move = tt_entry->bestmove;
        tt_depth = tt_entry->depth;
        tt_bound = tt_entry_bound(tt_entry);

        // Check if we can directly return a score for non-PV nodes.
        if (tt_depth >= depth && !pv_node
            && (((tt_bound & LOWER_BOUND) && tt_score >= beta)
                || ((tt_bound & UPPER_BOUND) && tt_score <= alpha))) {
            if ((tt_bound & LOWER_BOUND) && !board_move_is_noisy(board, tt_move)) {
                update_quiet_history(board, depth, tt_move, NULL, 0, ss);
            }

            return tt_score;
        }

        tt_noisy = tt_move != NO_MOVE && board_move_is_noisy(board, tt_move);
    }

    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;
    ss->double_extensions = (ss - 1)->double_extensions;

    const bool in_check = !!board->stack->checkers;
    bool improving;

    // Don't perform early pruning or compute the eval while in check.
    if (in_check) {
        eval = ss->static_eval = raw_eval = NO_SCORE;
        improving = false;
        goto main_loop;
    }
    // Use the TT stored information for getting an eval.
    else if (tt_found) {
        raw_eval = tt_entry->eval;
        eval = ss->static_eval = raw_eval
            + correction_hist_score(worker->correction_hist,
                                    board->side_to_move,
                                    board_pawn_key(board));

        // Try to use the TT score as a better evaluation of the position.
        if (tt_bound & (tt_score > eval ? LOWER_BOUND : UPPER_BOUND)) {
            eval = tt_score;
        }
    }
    // Call the evaluation function otherwise.
    else {
        raw_eval = evaluate(board);
        eval = ss->static_eval = raw_eval
            + correction_hist_score(worker->correction_hist,
                                    board->side_to_move,
                                    board_pawn_key(board));

        // Save the eval in TT so that other workers won't have to recompute it.
        tt_save(&worker->pool->tt, tt_entry, key, NO_SCORE, raw_eval, 0, NO_BOUND, NO_MOVE);
    }

    improving = ss->plies >= 2 && ss->static_eval > (ss - 2)->static_eval;

    // Replace the TT move by the nth best move from the previous search iteration in MultiPV
    // scenarios.
    if (root_node && worker->pv_line != 0) {
        tt_move = worker->root_moves[worker->pv_line].move;
    }

    // Razoring. If our static eval isn't good, and depth is low, it is likely that only a capture
    // will save us at this stage. Drop into qsearch.
    if (!pv_node && depth == 1 && ss->static_eval + 144 <= alpha) {
        return qsearch(false, board, alpha, beta, ss);
    }

    // Futility Pruning. If our eval is quite good and depth is low, we just assume that we won't
    // fall far behind in the next plies, and we return the eval.
    if (!pv_node && depth <= 7 && eval - 86 * depth + 79 * improving >= beta && eval < VICTORY) {
        return eval;
    }

    // Null Move Pruning. If our eval currently beats beta, and we still have non-Pawn material on
    // the board, we try to see what happens if we skip our turn. If the resulting reduced search
    // still beats beta, we assume our position is so good that we cannot get under beta at this
    // point.
    if (!pv_node && depth >= 3 && ss->plies >= worker->nmp_verif_plies && !ss->excluded_move
        && eval >= beta && eval >= ss->static_eval
        && board->stack->material[board->side_to_move] != 0) {
        Boardstack stack;
        Score score, verif_score;

        // Compute the depth reduction based on depth and eval difference with beta.
        i16 r = (855 + 61 * depth) / 256 + i16_min((eval - beta) / 111, 5);

        ss->current_move = NULL_MOVE;
        ss->piece_history = NULL;

        board_do_null_move(board, &stack);
        prefetch(tt_entry_at(&worker->pool->tt, board->stack->board_key));
        worker_increment_nodes(worker);

        // Perform the reduced search.
        score = -search(false, board, depth - r, -beta, -beta + 1, ss + 1, !cut_node);
        board_undo_null_move(board);

        if (score >= beta) {
            // Do not trust mate claims, as we don't want to return false mate scores due to
            // zugzwang. Adjust the score accordingly.
            if (score >= MATE_FOUND) {
                score = beta;
            }

            // Do not trust win claims for the same reason as above, and do not return early for
            // high-depth searches.
            if (worker->nmp_verif_plies != 0 || (depth <= 12 && i16_abs(beta) < VICTORY)) {
                return score;
            }

            // Verification search. For high depth nodes, we perform a second reduced search at the
            // same depth, but this time with NMP disabled for a few plies. If this search still
            // beats beta, we assume to not be in a zugzwang situation, and return the previous
            // reduced search score.
            worker->nmp_verif_plies = ss->plies + (depth - r) * 3 / 4;
            verif_score = search(false, board, depth - r, beta - 1, beta, ss, false);
            worker->nmp_verif_plies = 0;

            if (verif_score >= beta) {
                return score;
            }
        }
    }

    // Probcut. If we have a good enough capture (or promotion) and a reduced search returns a value
    // much above beta, we can (almost) safely prune the previous move.
    const Score probcut_beta = beta + 152;

    if (!root_node && depth >= 6 && i16_abs(beta) < VICTORY
        && !(tt_found && tt_depth >= depth - 4 && tt_score < probcut_beta)) {
        const Score probcut_see = probcut_beta - ss->static_eval;
        Move currmove;
        Boardstack stack;

        movepicker_init(
            &mp,
            true,
            board,
            worker,
            (tt_move && board_see_above(board, tt_move, probcut_see)) ? tt_move : NO_MOVE,
            ss
        );

        while ((currmove = movepicker_next_move(&mp, false, probcut_see)) != NO_MOVE) {
            if (mp.stage == PICK_BAD_NOISY) {
                break;
            }

            if (currmove == ss->excluded_move || !board_move_is_legal(board, currmove)) {
                continue;
            }

            ss->current_move = currmove;
            ss->piece_history =
                &worker->continuation_hist
                     ->piece_history[board_piece_on(board, move_from(currmove))][move_to(currmove)];

            board_do_move(board, currmove, &stack);
            prefetch(tt_entry_at(&worker->pool->tt, board->stack->board_key));
            worker_increment_nodes(worker);

            Score probcut_score = -qsearch(false, board, -probcut_beta, -probcut_beta + 1, ss + 1);

            if (probcut_score >= probcut_beta) {
                probcut_score = -search(
                    false,
                    board,
                    depth - 4,
                    -probcut_beta,
                    -probcut_beta + 1,
                    ss + 1,
                    !cut_node
                );
            }

            board_undo_move(board, currmove);

            if (probcut_score >= probcut_beta) {
                tt_save(
                    &worker->pool->tt,
                    tt_entry,
                    key,
                    score_to_tt(probcut_score, ss->plies),
                    raw_eval,
                    depth - 3,
                    LOWER_BOUND,
                    currmove
                );
                return probcut_score;
            }
        }
    }

    // Reduce depth if the node is absent from TT.
    if (!root_node && !tt_found && depth >= 3) {
        --depth;
    }

main_loop:
    movepicker_init(&mp, false, board, worker, tt_move, ss);

    Boardstack stack;
    Move currmove;
    Move bestmove = NO_MOVE;
    i16 move_count = 0;
    Move tried_quiets[64];
    i16 quiet_count = 0;
    Move tried_noisy[64];
    i16 noisy_count = 0;
    bool skip_quiets = false;

    while ((currmove = movepicker_next_move(&mp, skip_quiets, 0)) != NO_MOVE) {
        if (root_node) {
            // Exclude already searched PV lines for root nodes.
            if (find_root_move(
                    worker->root_moves + worker->pv_line,
                    worker->root_move_count - worker->pv_line,
                    currmove
                )
                == NULL) {
                continue;
            }
        } else if (currmove == ss->excluded_move || !board_move_is_legal(board, currmove)) {
            continue;
        }

        ++move_count;

        const bool is_quiet = !board_move_is_noisy(board, currmove);

        if (!root_node && best_score > -MATE_FOUND) {
            // Late Move Pruning. For low-depth nodes, stop searching quiets after a certain
            // movecount has been reached.
            if (depth <= 10 && move_count >= lmp_threshold(depth, improving)) {
                skip_quiets = true;
            }

            // Futility Pruning. For low-depth nodes, stop searching quiets if the eval suggests
            // that only captures will save the day.
            if (depth <= 5 && !in_check && is_quiet && eval + 186 + 66 * depth <= alpha) {
                skip_quiets = true;
            }

            // Continuation History Pruning. For low-depth nodes, prune quiet moves if they seem to
            // be bad continuations to the previous moves.
            if (depth <= 4
                && get_conthist_move_score(board, ss, currmove) < 783 - 4872 * (depth - 1)) {
                continue;
            }

            // SEE Pruning. For low-depth nodes, don't search moves which seem to lose too much
            // material to be interesting.
            if (depth <= 12
                && !board_see_above(
                    board,
                    currmove,
                    (is_quiet ? -48 * depth : -23 * depth * depth)
                )) {
                continue;
            }
        }

        // Report currmove info if enough time has passed.
        if (root_node && worker->thread_index == 0
            && timepoint_diff(worker->pool->timeman.start, timepoint_now()) >= 3000) {
            print_currmove(board, depth, currmove, move_count + (i16)worker->pv_line);
        }

        Score score = -INF_SCORE;
        i16 extension = 0;
        const i16 new_depth = depth - 1;
        const bool gives_check = board_move_gives_check(board, currmove);
        const i32 hist_score = is_quiet ? get_move_history_score(board, worker, ss, currmove) : 0;
        const Piece moved_piece = board_moved_piece(board, currmove);

        if (!root_node && ss->plies < 2 * (i16)worker->root_depth
            && 2 * ss->double_extensions < (i16)worker->root_depth) {
            // Singular Extensions. For high-depth nodes, if the TT entry suggests that the TT move
            // is really good, we check if there are other moves which maintain the score close to
            // the TT score. If that's not the case, we consider the TT move to be singular, and we
            // extend non-LMR searches by one or two lies, depending on the margin that the singular
            // search failed low.
            if (depth >= 7 && currmove == tt_move && !ss->excluded_move && (tt_bound & LOWER_BOUND)
                && i16_abs(tt_score) < VICTORY && tt_depth >= depth - 3) {
                const Score singular_beta = tt_score - 10 * depth / 16;
                const i16 singular_depth = depth / 2 + 1;
                Score singular_score;

                // Exclude the TT move from the singular search.
                ss->excluded_move = tt_move;
                singular_score = search(
                    false,
                    board,
                    singular_depth,
                    singular_beta - 1,
                    singular_beta,
                    ss,
                    cut_node
                );
                ss->excluded_move = NO_MOVE;

                // If our singular search failed to produce a cutoff, extend the TT move.
                if (singular_score < singular_beta) {
                    if (!pv_node && singular_beta - singular_score > 14
                        && ss->double_extensions <= 10) {
                        extension = 2 + (!tt_noisy && singular_beta - singular_score > 120);
                        ++ss->double_extensions;
                    } else {
                        extension = 1;
                    }
                }
                // Multicut Pruning. If our singular search produced a cutoff, and the search bounds
                // were equal or superior to our normal search, assume that there are multiple moves
                // that beat beta in the current node, and return a search score early.
                else if (singular_beta >= beta) {
                    return singular_beta;
                }
                // Negative Extensions. If our singular search produced a cutoff, with singularBeta
                // was too low to beat beta, but the TT entry having a search score above beta, we
                // assume that searching the TT move at full depth is futile as we should get a
                // fail-high deeper on this branch, and reduce its search depth.
                else if (tt_score >= beta) {
                    extension = -1;
                }
            }
            // Check Extensions. Extend non-LMR searches by one ply for moves that give check.
            else if (gives_check) {
                extension = 1;
            }
        }

        // Save the piece history for the current move so that sub-nodes can use it for ordering
        // moves.
        ss->current_move = currmove;
        ss->piece_history =
            &worker->continuation_hist->piece_history[moved_piece][move_to(currmove)];

        board_do_move_gc(board, currmove, &stack, gives_check);
        prefetch(tt_entry_at(&worker->pool->tt, board->stack->board_key));
        worker_increment_nodes(worker);

        // Late Move Reductions. For nodes not too close to qsearch (since we can't reduce their
        // search depth), we start reducing moves after a certain movecount has been reached, as we
        // consider them less likely to produce cutoffs in standard searches.
        if (depth >= 3 && move_count > 1 + 3 * pv_node) {
            // Set the base depth reduction value based on depth and movecount.
            i16 r = lmr_base_value(depth, move_count, improving, is_quiet);

            // Increase the reduction for non-PV nodes.
            r += !pv_node;

            // Increase the reduction for cutNodes.
            r += cut_node;

            // Increase the reduction if the TT move is non-quiet.
            r += tt_noisy;

            // Decrease the reduction if the move is a killer or countermove.
            r -= (currmove == mp.killer1 || currmove == mp.killer2 || currmove == mp.counter);

            // Decrease the reduction if the move escapes a capture.
            r -= is_quiet && !board_see_above(board, move_reverse(currmove), 0);

            // Increase/decrease the reduction based on the move's history.
            r -= (i16)i32_clamp(hist_score / 11601, -3, 3);

            // Clamp the reduction so that we don't extend the move or drop
            // immediately into qsearch.
            r = i16_clamp(r, 0, new_depth - 1);

            score = -search(false, board, new_depth - r, -alpha - 1, -alpha, ss + 1, true);

            // Perform another search at full depth if LMR failed high.
            if (r != 0 && score > alpha) {
                score = -search(
                    false,
                    board,
                    new_depth + extension,
                    -alpha - 1,
                    -alpha,
                    ss + 1,
                    !cut_node
                );
                update_continuation_histories(
                    ss,
                    depth,
                    moved_piece,
                    move_to(currmove),
                    score > alpha
                );
            }
        }
        // If LMR is not possible, do a search with no reductions.
        else if (!pv_node || move_count != 1) {
            score =
                -search(false, board, new_depth + extension, -alpha - 1, -alpha, ss + 1, !cut_node);
        }

        // In PV nodes, perform an additional full-window search for the first move, or when all our
        // previous searches returned fail-highs.
        if (pv_node && (move_count == 1 || score > alpha)) {
            pv_line_init(&ss->pv);
            score = -search(true, board, new_depth + extension, -beta, -alpha, ss + 1, false);
        }

        board_undo_move(board, currmove);

        // Check for search interruption here.
        if (wpool_is_stopped(worker->pool)) {
            return 0;
        }

        if (root_node) {
            RootMove *cur_root_move = find_root_move(
                worker->root_moves + worker->pv_line,
                worker->root_move_count - worker->pv_line,
                currmove
            );

            // Update the PV in root nodes for the first move, and for all subsequent moves beating
            // alpha.
            if (move_count == 1 || score > alpha) {
                cur_root_move->score = score;
                cur_root_move->seldepth = worker->seldepth;
                pv_line_update(&cur_root_move->pv, currmove, &ss->pv);
            } else {
                cur_root_move->score = -INF_SCORE;
            }
        }

        best_score = (Score)i16_max(best_score, score);

        // Check if our score beats alpha.
        if (alpha < best_score) {
            bestmove = currmove;
            alpha = best_score;

            // Backpropagate the PV line if needed.
            if (pv_node && !root_node) {
                pv_line_update(&(ss - 1)->pv, currmove, &ss->pv);
            }

            // Check if our move generates a cutoff, in which case we can stop searching other moves
            // at this node. Additionally update move histories.
            if (alpha >= beta) {
                if (is_quiet) {
                    update_quiet_history(board, depth, bestmove, tried_quiets, quiet_count, ss);
                }

                if (move_count != 1) {
                    update_capture_history(board, depth, bestmove, tried_noisy, noisy_count, ss);
                }

                break;
            }
        }

        // Keep track of all moves that failed to generate a cutoff.
        if (quiet_count < 64 && is_quiet) {
            tried_quiets[quiet_count++] = currmove;
        } else if (noisy_count < 64 && !is_quiet) {
            tried_noisy[noisy_count++] = currmove;
        }
    }

    // Are we in checkmate/stalemate ? Take care of not returning a wrong draw
    // or mate score in singular searches.
    if (move_count == 0) {
        best_score = ss->excluded_move ? alpha : in_check ? mated_in((u8)ss->plies) : DRAW;
    }

    Bound bound = (best_score >= beta) ? LOWER_BOUND
        : (pv_node && bestmove)        ? EXACT_BOUND
                                       : UPPER_BOUND;

    // If we're not in check, and we don't have a tactical best-move, and the static eval needs
    // moving in a direction, then update corrhist.
    if (!(in_check || (bestmove && board_move_is_noisy(board, bestmove))
          || (bound == LOWER_BOUND && best_score <= ss->static_eval)
          || (bound == UPPER_BOUND && best_score >= ss->static_eval))) {
        correction_hist_update(
            worker->correction_hist,
            board->side_to_move,
            board_pawn_key(board),
            i16_min(16, depth + 1),
            (i32)best_score - (i32)ss->static_eval
        );
    }

    // Only save TT for the first MultiPV move in root nodes.
    if (!root_node || worker->pv_line == 0) {
        tt_save(
            &worker->pool->tt,
            tt_entry,
            key,
            score_to_tt(best_score, ss->plies),
            raw_eval,
            depth,
            bound,
            bestmove
        );
    }

    return best_score;
}

Score qsearch(bool pv_node, Board *board, Score alpha, Score beta, Searchstack *ss) {
    Worker *worker = board_get_worker(board);
    const Score old_alpha = alpha;
    Movepicker mp;

    // Verify the time usage if we're the main thread.
    if (worker->thread_index == 0) {
        wpool_check_time(worker->pool);
    }

    // Update the seldepth value if needed.
    if (pv_node) {
        worker->seldepth = u16_max(worker->seldepth, ss->plies + 1);
    }

    // Stop the search if the game is drawn or the timeman/UCI thread asks for a stop.
    if (wpool_is_stopped(worker->pool) || board_game_is_drawn(board, ss->plies)) {
        return worker_draw_score(worker);
    }

    // Stop the search after MAX_PLIES recursive search calls.
    if (ss->plies >= MAX_PLIES) {
        return !board->stack->checkers ? evaluate(board) : worker_draw_score(worker);
    }

    // Mate distance pruning
    alpha = i16_max(alpha, mated_in(ss->plies));
    beta = i16_min(beta, mate_in(ss->plies + 1));

    if (alpha >= beta) {
        return alpha;
    }

    Score tt_score = NO_SCORE;
    Bound tt_bound = NO_BOUND;
    Move tt_move = NO_MOVE;
    bool tt_found;
    TranspositionEntry *tt_entry;

    tt_entry = tt_probe(&worker->pool->tt, board->stack->board_key, &tt_found);

    // Probe the TT for information on the current position.
    if (tt_found) {
        tt_score = score_from_tt(tt_entry->score, ss->plies);
        tt_bound = tt_entry_bound(tt_entry);
        tt_move = tt_entry->bestmove;

        // Check if we can directly return a score for non-PV nodes.
        if (!pv_node
            && (((tt_bound & LOWER_BOUND) && tt_score >= beta)
                || ((tt_bound & UPPER_BOUND) && tt_score <= alpha))) {
            return tt_score;
        }
    }

    const bool in_check = !!board->stack->checkers;
    Score raw_eval;
    Score eval;
    Score best_score;

    // Don't compute the eval while in check.
    if (in_check) {
        raw_eval = eval = NO_SCORE;
        best_score = -INF_SCORE;
    } else {
        // Use the TT stored information for getting an eval.
        if (tt_found) {
            raw_eval = tt_entry->eval;
            eval = best_score = raw_eval
                + correction_hist_score(
                                    worker->correction_hist,
                                    board->side_to_move,
                                    board_pawn_key(board)
                );

            // Try to use the TT score as a better evaluation of the position.
            if (tt_bound & (tt_score > best_score ? LOWER_BOUND : UPPER_BOUND)) {
                best_score = tt_score;
            }
        }
        // Call the evaluation function otherwise.
        else {
            raw_eval = evaluate(board);
            eval = best_score = raw_eval
                + correction_hist_score(
                                    worker->correction_hist,
                                    board->side_to_move,
                                    board_pawn_key(board)
                );
        }

        // Stand Pat. If not playing a capture is better because of better quiet moves, allow for a
        // direct eval return.
        alpha = (Score)i16_max(alpha, best_score);

        if (alpha >= beta) {
            // Save the eval in TT so that other workers won't have to recompute it.
            if (!tt_found) {
                tt_save(
                    &worker->pool->tt,
                    tt_entry,
                    board->stack->board_key,
                    score_to_tt(best_score, ss->plies),
                    raw_eval,
                    0,
                    LOWER_BOUND,
                    NO_MOVE
                );
            }

            return alpha;
        }
    }

    movepicker_init(&mp, true, board, worker, tt_move, ss);

    Move currmove;
    Move bestmove = NO_MOVE;
    i16 move_count = 0;
    Boardstack stack;

    // Check if Futility Pruning is possible in the moves loop.
    const bool futility_ok = !in_check && board_total_piece_count(board) >= 5;
    const Score futility_base = best_score + 98;

    while ((currmove = movepicker_next_move(&mp, false, 0)) != NO_MOVE) {
        // Only analyse good noisy moves.
        if (best_score > -MATE_FOUND && mp.stage == PICK_BAD_NOISY) {
            break;
        }

        if (!board_move_is_legal(board, currmove)) {
            continue;
        }

        ++move_count;

        const bool gives_check = board_move_gives_check(board, currmove);

        // Futility Pruning. If we already have a non-mating score and our move doesn't give check,
        // test if playing it has a chance to make the score go over alpha.
        if (best_score > -MATE_FOUND && futility_ok && !gives_check
            && move_type(currmove) == NORMAL_MOVE) {
            const Score futility_value =
                futility_base + PieceScores[ENDGAME][board_piece_on(board, move_to(currmove))];

            // Check if the move is unlikely to improve alpha.
            if (futility_value < alpha) {
                continue;
            }

            // If static eval is far below alpha, only search moves that win material.
            if (futility_base < alpha && !board_see_above(board, currmove, 1)) {
                continue;
            }
        }

        // Save the piece history for the current move so that sub-nodes can use it for ordering
        // moves.
        ss->current_move = currmove;
        ss->piece_history =
            &worker->continuation_hist
                 ->piece_history[board_moved_piece(board, currmove)][move_to(currmove)];

        if (pv_node) {
            pv_line_init(&ss->pv);
        }

        board_do_move_gc(board, currmove, &stack, gives_check);
        prefetch(tt_entry_at(&worker->pool->tt, board->stack->board_key));
        worker_increment_nodes(worker);

        Score score = -qsearch(pv_node, board, -beta, -alpha, ss + 1);

        board_undo_move(board, currmove);

        // Check for search interruption here.
        if (wpool_is_stopped(worker->pool)) {
            return 0;
        }

        best_score = (Score)i16_max(best_score, score);

        // Check if our score beats alpha.
        if (alpha < best_score) {
            alpha = best_score;
            bestmove = currmove;

            // Update the PV for PV nodes.
            if (pv_node) {
                pv_line_update(&(ss - 1)->pv, bestmove, &ss->pv);
            }

            // Check if our move generates a cutoff, in which case we can stop searching other moves
            // at this node.
            if (alpha >= beta) {
                break;
            }
        }
    }

    // Are we checkmated ?
    if (move_count == 0 && in_check) {
        best_score = mated_in(ss->plies);
    }

    Bound bound = (best_score >= beta) ? LOWER_BOUND
        : (best_score <= old_alpha)    ? UPPER_BOUND
                                       : EXACT_BOUND;

    tt_save(
        &worker->pool->tt,
        tt_entry,
        board->stack->board_key,
        score_to_tt(best_score, ss->plies),
        raw_eval,
        0,
        bound,
        bestmove
    );
    return best_score;
}

void update_continuation_histories(
    Searchstack *ss,
    i16 depth,
    Piece piece,
    Square to,
    bool fail_high
) {
    i16 bonus = history_bonus(depth);

    if (!fail_high) {
        bonus = -bonus;
    }

    if ((ss - 1)->piece_history != NULL) {
        piece_hist_update((ss - 1)->piece_history, piece, to, bonus);
    }

    if ((ss - 2)->piece_history != NULL) {
        piece_hist_update((ss - 2)->piece_history, piece, to, bonus);
    }

    if ((ss - 4)->piece_history != NULL) {
        piece_hist_update((ss - 4)->piece_history, piece, to, bonus);
    }
}

void update_quiet_history(
    const Board *board,
    i16 depth,
    Move bestmove,
    const Move *tried_quiets,
    i16 quiet_count,
    Searchstack *ss
) {
    Worker *worker = board_get_worker(board);
    const i16 bonus = history_bonus(depth);
    const Move previous_move = (ss - 1)->current_move;
    Piece moved_piece = board_moved_piece(board, bestmove);
    Square to = move_to(bestmove);

    // Apply history bonuses to the bestmove.
    if ((ss - 1)->piece_history != NULL) {
        const Square last_to = move_to(previous_move);
        const Piece last_piece = board_piece_on(board, last_to);

        worker->counter_hist->data[last_piece][last_to] = bestmove;
    }

    butterfly_hist_update(worker->butterfly_hist, moved_piece, bestmove, bonus);
    update_continuation_histories(ss, depth, moved_piece, to, true);

    // Set the bestmove as a killer.
    if (ss->killers[0] != bestmove) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestmove;
    }

    // Apply history penalties to all previous failing quiet moves.
    for (i16 i = 0; i < quiet_count; ++i) {
        moved_piece = board_moved_piece(board, tried_quiets[i]);
        to = move_to(tried_quiets[i]);
        butterfly_hist_update(worker->butterfly_hist, moved_piece, tried_quiets[i], -bonus);
        update_continuation_histories(ss, depth, moved_piece, to, false);
    }
}

static void
    update_single_capture(CaptureHistory *capture_hist, const Board *board, Move move, i16 bonus) {
    const Piece moved_piece = board_moved_piece(board, move);
    const Square to = move_to(move);
    const Piecetype captured = (move_type(move) == PROMOTION) ? move_promotion_type(move)
        : (move_type(move) == EN_PASSANT)                     ? PAWN
                                          : piece_type(board_piece_on(board, to));

    capture_hist_update(capture_hist, moved_piece, to, captured, bonus);
}

void update_capture_history(
    const Board *board,
    i16 depth,
    Move bestmove,
    const Move *tried_noisy,
    i16 noisy_count,
    __attribute__((unused)) Searchstack *ss
) {
    CaptureHistory *capture_hist = board_get_worker(board)->capture_hist;
    const i16 bonus = history_bonus(depth);

    // Apply history bonuses to the bestmove.
    if (board_move_is_noisy(board, bestmove)) {
        update_single_capture(capture_hist, board, bestmove, bonus);
    }

    // Apply history penalties to all previous failing capture moves.
    for (i16 i = 0; i < noisy_count; ++i) {
        update_single_capture(capture_hist, board, tried_noisy[i], -bonus);
    }
}
