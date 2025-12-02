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

static void movepicker_score_noisy(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end) {
    static const i32 CapturedBonus[PIECETYPE_NB] = {0, 0, 1280, 1280, 2560, 5120, 0, 0};

    while (begin < end) {
        const Move move = begin->move;
        const Square to = move_to(move);
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        Piecetype captured = piece_type(board_piece_on(mp->board, to));

        // Give an additional bonus for promotions based on the promotion type.
        if (move_type(move) == PROMOTION) {
            begin->score = CapturedBonus[move_promotion_type(move)];
            captured = move_promotion_type(move);
        }
        // Special case for en-passant captures, since the arrival square is empty.
        else if (move_type(move) == EN_PASSANT) {
            begin->score = CapturedBonus[PAWN];
            captured = PAWN;
        } else {
            begin->score = CapturedBonus[captured];
        }

        // In addition to the MVV ordering, rank the captures based on their history.
        begin->score += capture_hist_score(mp->worker->capture_hist, moved_piece, to, captured);
        ++begin;
    }
}

static void movepicker_score_quiets(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end) {
    while (begin < end) {
        const Move move = begin->move;
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        const Square to = move_to(move);

        begin->score = butterfly_hist_score(mp->worker->butterfly_hist, moved_piece, move) / 2;
        begin->score += piece_hist_score(mp->piece_history[0], moved_piece, to);
        begin->score += piece_hist_score(mp->piece_history[1], moved_piece, to);

        ++begin;
    }
}

static void movepicker_score_evasions(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end) {
    while (begin < end) {
        const Move move = begin->move;
        const Piece moved_piece = board_piece_on(mp->board, move_from(move));
        const Square to = move_to(move);

        if (board_move_is_noisy(mp->board, move)) {
            const Piecetype captured = piece_type(board_piece_on(mp->board, to));

            // Place captures of the checking piece at the top of the list using MVV/LVA ordering.
            begin->score = 65536 + captured * 8 - piece_type(moved_piece);
        } else {
            begin->score = butterfly_hist_score(mp->worker->butterfly_hist, moved_piece, move) / 2;
            begin->score += piece_hist_score(mp->piece_history[0], moved_piece, to);
            begin->score += piece_hist_score(mp->piece_history[1], moved_piece, to);
        }

        ++begin;
    }
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
            mp->list.end = extmove_generate_noisy(mp->list.moves, mp->board, mp->in_qsearch);
            movepicker_score_noisy(mp, mp->list.moves, mp->list.end);
            mp->current = mp->bad_captures = mp->list.moves;
            // Fallthrough

        case PICK_GOOD_NOISY:
            while (mp->current < mp->list.end) {
                extmove_pick_best(mp->current, mp->list.end);

                // Only select moves with a SEE above the required threshold for this stage.
                if (mp->current->move != mp->tt_move
                    && board_see_above(mp->board, mp->current->move, see_threshold)) {
                    return (mp->current++)->move;
                }

                // Keep track of bad captures for later.
                *(mp->bad_captures++) = *(mp->current++);
            }

            // If we're in qsearch, we skip quiet move generation/selection when not in check.
            if (mp->in_qsearch) {
                mp->current = mp->list.moves;
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
                mp->list.end = extmove_generate_quiet(mp->current, mp->board);
                movepicker_score_quiets(mp, mp->current, mp->list.end);
            }

            // Fallthrough

        case PICK_QUIETS:
            // Stop picking quiets if the search tells us to do so due to quiet move pruning.
            if (!skip_quiets) {
                while (mp->current < mp->list.end) {
                    extmove_pick_best(mp->current, mp->list.end);

                    const Move move = (mp->current++)->move;

                    // Don't play the same move twice.
                    if (move != mp->tt_move && move != mp->killer && move != mp->counter) {
                        return move;
                    }
                }
            }

            ++mp->stage;
            mp->current = mp->list.moves;
            // Fallthrough

        case PICK_BAD_NOISY:
            // Select all remaining captures. Note that we have already ordered them in the
            // PICK_GOOD_NOISY stage.
            while (mp->current < mp->bad_captures) {
                const Move move = (mp->current++)->move;

                if (move != mp->tt_move) {
                    return move;
                }
            }

            break;

        case CHECK_GEN_ALL:
            // Generate and score all evasions.
            ++mp->stage;
            mp->list.end = extmove_generate_incheck(mp->list.moves, mp->board);
            movepicker_score_evasions(mp, mp->list.moves, mp->list.end);
            mp->current = mp->list.moves;
            // Fallthrough

        case CHECK_PICK_ALL:
            // Select the next best evasion.
            while (mp->current < mp->list.end) {
                extmove_pick_best(mp->current, mp->list.end);

                const Move move = (mp->current++)->move;

                if (move != mp->tt_move) {
                    return move;
                }
            }

            break;
    }

    // We went through all stages, so we can inform the search that there are no moves left to pick.
    return NO_MOVE;
}
