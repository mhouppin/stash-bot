/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#include "movepicker.h"

void movepicker_init(
    Movepicker *mp,
    bool in_qsearch,
    const Board *board,
    const Worker *worker,
    Move tt_move,
    Searchstack *ss
) {
    mp->in_qsearch = in_qsearch;

    // We use a special ordering method when we are in check.
    if (board->stack->checkers) {
        mp->stage =
            CHECK_PICK_TT + !(tt_move != NO_MOVE && board_move_is_pseudolegal(board, tt_move));
    } else {
        mp->stage = PICK_TT
            + !(tt_move != NO_MOVE && (!in_qsearch || board_move_is_noisy(board, tt_move))
                && board_move_is_pseudolegal(board, tt_move));
    }

    mp->tt_move = tt_move;
    mp->killer = ss->killer;

    // Set the countermove if possible.
    if ((ss - 1)->piece_history != NULL) {
        const Square last_to = move_to((ss - 1)->current_move);
        const Piece last_piece = board_piece_on(board, last_to);

        mp->counter = worker->counter_hist->data[last_piece][last_to];
    } else {
        mp->counter = NO_MOVE;
    }

    mp->piece_history[0] = (ss - 1)->piece_history;
    mp->piece_history[1] = (ss - 2)->piece_history;
    mp->board = board;
    mp->worker = worker;
}

static void movepicker_score_noisy(Movepicker *mp, Move *movelist, i32 *score_list, usize size) {
    static const i32 CapturedBonus[PIECETYPE_NB] = {0, 0, 1280, 1280, 2560, 5120, 0, 0};

    for (usize i = 0; i < size; ++i) {
        const Move move = movelist[i];
        const Square to = move_to(move);
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        Piecetype captured = piece_type(board_piece_on(mp->board, to));

        // Give an additional bonus for promotions based on the promotion type.
        if (move_type(move) == PROMOTION) {
            score_list[i] = CapturedBonus[move_promotion_type(move)];
            captured = move_promotion_type(move);
        }
        // Special case for en-passant captures, since the arrival square is empty.
        else if (move_type(move) == EN_PASSANT) {
            score_list[i] = CapturedBonus[PAWN];
            captured = PAWN;
        } else {
            score_list[i] = CapturedBonus[captured];
        }

        // In addition to the MVV ordering, rank the captures based on their history.
        score_list[i] += capture_hist_score(mp->worker->capture_hist, moved_piece, to, captured);
    }
}

static void movepicker_score_quiets(Movepicker *mp, Move *movelist, i32 *score_list, usize size) {
    for (usize i = 0; i < size; ++i) {
        const Move move = movelist[i];
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        const Square to = move_to(move);

        score_list[i] = butterfly_hist_score(mp->worker->butterfly_hist, moved_piece, move) / 2
            + piece_hist_score(mp->piece_history[0], moved_piece, to)
            + piece_hist_score(mp->piece_history[1], moved_piece, to);
    }
}

static void movepicker_score_evasions(Movepicker *mp, Move *movelist, i32 *score_list, usize size) {
    for (usize i = 0; i < size; ++i) {
        const Move move = movelist[i];
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        const Square to = move_to(move);

        if (board_move_is_noisy(mp->board, move)) {
            const Piecetype captured = piece_type(board_piece_on(mp->board, to));

            // Place captures of the checking piece at the top of the list using MVV/LVA ordering.
            score_list[i] = 65536 + captured * 8 - piece_type(moved_piece);
        } else {
            score_list[i] = butterfly_hist_score(mp->worker->butterfly_hist, moved_piece, move) / 2
                + piece_hist_score(mp->piece_history[0], moved_piece, to)
                + piece_hist_score(mp->piece_history[1], moved_piece, to);
        }
    }
}

static inline Move mp_get_move(const Movepicker *mp) {
    return mp->move_list[mp->current_idx];
}

static inline Move mp_yield_move(Movepicker *mp) {
    return mp->move_list[mp->current_idx++];
}

static inline void mp_push_bad_capture(Movepicker *mp) {
    mp->score_list[mp->bad_captures_idx] = mp->score_list[mp->current_idx];
    mp->move_list[mp->bad_captures_idx] = mp->move_list[mp->current_idx];
    ++mp->bad_captures_idx;
    ++mp->current_idx;
}

Move movepicker_next_move(Movepicker *mp, bool skip_quiets, Score see_threshold) {
top:
    switch (mp->stage) {
        case PICK_TT:
        case CHECK_PICK_TT:
            // Pseudo-legality has already been verified, return the TT move.
            ++mp->stage;
            return mp->tt_move;

        case GEN_NOISY:
            // Generate and score all noisy moves.
            ++mp->stage;
            mp->move_count =
                (usize)(extmove_generate_noisy(mp->move_list, mp->board, mp->in_qsearch)
                        - mp->move_list);
            movepicker_score_noisy(mp, mp->move_list, mp->score_list, mp->move_count);
            mp->current_idx = mp->bad_captures_idx = 0;
            // Fallthrough

        case PICK_GOOD_NOISY:
            while (mp->current_idx < mp->move_count) {
                extmove_pick_best(
                    mp->move_list + mp->current_idx,
                    mp->score_list + mp->current_idx,
                    mp->move_count - mp->current_idx
                );

                // Only select moves with a SEE above the required threshold for this stage.
                if (mp_get_move(mp) != mp->tt_move
                    && board_see_above(mp->board, mp_get_move(mp), see_threshold)) {
                    return mp_yield_move(mp);
                }

                // Keep track of bad captures for later.
                mp_push_bad_capture(mp);
            }

            // If we're in qsearch, we skip quiet move generation/selection when not in check.
            if (mp->in_qsearch) {
                mp->current_idx = 0;
                mp->stage = PICK_BAD_NOISY;
                goto top;
            }

            ++mp->stage;
            // Fallthrough

        case PICK_KILLER:
            ++mp->stage;

            // Don't play the same move twice.
            if (mp->killer != NO_MOVE && mp->killer != mp->tt_move
                && !board_move_is_noisy(mp->board, mp->killer)
                && board_move_is_pseudolegal(mp->board, mp->killer)) {
                return mp->killer;
            }

            // Fallthrough

        case PICK_COUNTER:
            ++mp->stage;

            // Don't play the same move twice.
            if (mp->counter != NO_MOVE && mp->counter != mp->tt_move && mp->counter != mp->killer
                && !board_move_is_noisy(mp->board, mp->counter)
                && board_move_is_pseudolegal(mp->board, mp->counter)) {
                return mp->counter;
            }

            // Fallthrough

        case GEN_QUIETS:
            // Generate and score all quiet moves, except if the search tells us to not do so due to
            // quiet move pruning.
            ++mp->stage;

            if (!skip_quiets) {
                mp->move_count =
                    (usize)(extmove_generate_quiet(mp->move_list + mp->current_idx, mp->board)
                            - mp->move_list);
                movepicker_score_quiets(
                    mp,
                    mp->move_list + mp->current_idx,
                    mp->score_list + mp->current_idx,
                    mp->move_count - mp->current_idx
                );
            }

            // Fallthrough

        case PICK_QUIETS:
            // Stop picking quiets if the search tells us to do so due to quiet move pruning.
            if (!skip_quiets) {
                while (mp->current_idx < mp->move_count) {
                    extmove_pick_best(
                        mp->move_list + mp->current_idx,
                        mp->score_list + mp->current_idx,
                        mp->move_count - mp->current_idx
                    );

                    const Move move = mp_yield_move(mp);

                    // Don't play the same move twice.
                    if (move != mp->tt_move && move != mp->killer && move != mp->counter) {
                        return move;
                    }
                }
            }

            ++mp->stage;
            mp->current_idx = 0;
            // Fallthrough

        case PICK_BAD_NOISY:
            // Select all remaining captures. Note that we have already ordered them in the
            // PICK_GOOD_NOISY stage.
            while (mp->current_idx < mp->bad_captures_idx) {
                const Move move = mp_yield_move(mp);

                if (move != mp->tt_move) {
                    return move;
                }
            }

            break;

        case CHECK_GEN_ALL:
            // Generate and score all evasions.
            ++mp->stage;
            mp->move_count =
                (usize)(extmove_generate_incheck(mp->move_list, mp->board) - mp->move_list);
            movepicker_score_evasions(mp, mp->move_list, mp->score_list, mp->move_count);
            mp->current_idx = 0;
            // Fallthrough

        case CHECK_PICK_ALL:
            // Select the next best evasion.
            while (mp->current_idx < mp->move_count) {
                extmove_pick_best(
                    mp->move_list + mp->current_idx,
                    mp->score_list + mp->current_idx,
                    mp->move_count - mp->current_idx
                );

                const Move move = mp_yield_move(mp);

                if (move != mp->tt_move) {
                    return move;
                }
            }

            break;
    }

    // We went through all stages, so we can inform the search that there are no moves left to pick.
    return NO_MOVE;
}
