/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#include "movepick.h"

void movepicker_init(Movepicker *mp, bool inQsearch, const Board *board, const worker_t *worker,
    move_t ttMove, Searchstack *ss)
{
    mp->inQsearch = inQsearch;

    // Use a special ordering method when we're in check.
    if (board->stack->checkers)
        mp->stage = CHECK_PICK_TT + !(ttMove && move_is_pseudo_legal(board, ttMove));
    else
        mp->stage = PICK_TT
                    + !(ttMove && (!inQsearch || is_capture_or_promotion(board, ttMove))
                        && move_is_pseudo_legal(board, ttMove));

    mp->ttMove = ttMove;
    mp->killer1 = ss->killers[0] ?: (ss - 2)->killers[0];
    mp->killer2 = ss->killers[1] ?: (ss - 2)->killers[!ss->killers[0]];

    // Load the countermove if the search stack from the last turn allows us to
    // do so.
    if ((ss - 1)->pieceHistory != NULL)
    {
        square_t lastTo = to_sq((ss - 1)->currentMove);
        square_t lastPiece = piece_on(board, lastTo);
        mp->counter = worker->cmHistory[lastPiece][lastTo];
    }
    else
        mp->counter = NO_MOVE;

    mp->pieceHistory[0] = (ss - 1)->pieceHistory;
    mp->pieceHistory[1] = (ss - 2)->pieceHistory;
    mp->board = board;
    mp->worker = worker;
}

static void score_captures(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end)
{
    static const score_t MVV_LVA[PIECETYPE_NB] = {0, 0, 640, 640, 1280, 2560, 0, 0};

    while (begin < end)
    {
        move_t move = begin->move;
        square_t to = to_sq(move);
        piece_t movedPiece = piece_on(mp->board, from_sq(move));
        piecetype_t captured = piece_type(piece_on(mp->board, to));

        // Give an additional bonus for promotions based on the promotion type.
        if (move_type(move) == PROMOTION)
        {
            begin->score = MVV_LVA[promotion_type(move)];
            captured = piece_type(promotion_type(move));
        }
        // Special case for en-passant captures, since the arrival square is
        // empty.
        else if (move_type(move) == EN_PASSANT)
        {
            begin->score = MVV_LVA[PAWN];
            captured = PAWN;
        }
        else
            begin->score = MVV_LVA[captured];

        // In addition to the MVV ordering, rank the captures based on their
        // history.
        begin->score += get_cap_history_score(mp->worker->capHistory, movedPiece, to, captured);

        ++begin;
    }
}

static void score_quiet(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end)
{
    while (begin < end)
    {
        piece_t moved = piece_on(mp->board, from_sq(begin->move));
        square_t to = to_sq(begin->move);

        // Start by using the butterfly history for ranking quiet moves.
        begin->score = get_bf_history_score(mp->worker->bfHistory, moved, begin->move) / 2;

        // Try using the countermove and followup histories if they exist.
        if (mp->pieceHistory[0] != NULL)
            begin->score += get_pc_history_score(*mp->pieceHistory[0], moved, to);
        if (mp->pieceHistory[1] != NULL)
            begin->score += get_pc_history_score(*mp->pieceHistory[1], moved, to);

        ++begin;
    }
}

static void score_evasions(Movepicker *mp, ExtendedMove *begin, ExtendedMove *end)
{
    while (begin < end)
    {
        if (is_capture_or_promotion(mp->board, begin->move))
        {
            piecetype_t moved = piece_type(piece_on(mp->board, from_sq(begin->move)));
            piecetype_t captured = piece_type(piece_on(mp->board, to_sq(begin->move)));

            // Place captures of the checking piece at the top of the list using
            // MVV/LVA ordering.
            begin->score = 28672 + captured * 8 - moved;
        }
        else
        {
            piece_t moved = piece_on(mp->board, from_sq(begin->move));
            square_t to = to_sq(begin->move);

            // Start by using the butterfly history for ranking quiet moves.
            begin->score = get_bf_history_score(mp->worker->bfHistory, moved, begin->move) / 2;

            // Try using the countermove and followup histories if they exist.
            if (mp->pieceHistory[0] != NULL)
                begin->score += get_pc_history_score(*mp->pieceHistory[0], moved, to);
            if (mp->pieceHistory[1] != NULL)
                begin->score += get_pc_history_score(*mp->pieceHistory[1], moved, to);
        }

        ++begin;
    }
}

move_t movepicker_next_move(Movepicker *mp, bool skipQuiets)
{
__top:

    switch (mp->stage)
    {
        case PICK_TT:
        case CHECK_PICK_TT:
            // Pseudo-legality has already been verified, return the TT move.
            ++mp->stage;
            return mp->ttMove;

        case GEN_INSTABLE:
            // Generate and score all capture moves.
            ++mp->stage;
            mp->list.last = generate_captures(mp->list.moves, mp->board, mp->inQsearch);
            score_captures(mp, mp->list.moves, mp->list.last);
            mp->cur = mp->badCaptures = mp->list.moves;
            // Fallthrough

        case PICK_GOOD_INSTABLE:
            while (mp->cur < mp->list.last)
            {
                place_top_move(mp->cur, mp->list.last);

                // Only select moves with a positive SEE for this stage.
                if (mp->cur->move != mp->ttMove && see_greater_than(mp->board, mp->cur->move, 0))
                    return (mp->cur++)->move;

                // Place bad captures further in the list so that we can use
                // them later.
                *(mp->badCaptures++) = *(mp->cur++);
            }

            // If we're in qsearch, we skip quiet move generation/selection when
            // not in check.
            if (mp->inQsearch)
            {
                mp->cur = mp->list.moves;
                mp->stage = PICK_BAD_INSTABLE;
                goto __top;
            }
            ++mp->stage;
            // Fallthrough

        case PICK_KILLER1:
            ++mp->stage;
            // Return the move only if it is pseudo-legal and different from the
            // TT move.
            if (mp->killer1 && mp->killer1 != mp->ttMove
                && move_is_pseudo_legal(mp->board, mp->killer1))
                return mp->killer1;
            // Fallthrough

        case PICK_KILLER2:
            ++mp->stage;
            // Return the move only if it is pseudo-legal and different from the
            // TT move and first killer.
            if (mp->killer2 && mp->killer2 != mp->ttMove && mp->killer2 != mp->killer1
                && move_is_pseudo_legal(mp->board, mp->killer2))
                return mp->killer2;
            // Fallthrough

        case PICK_COUNTER:
            ++mp->stage;
            // Return the move only if it is pseudo-legal and different from the
            // TT move and the two killers.
            if (mp->counter && mp->counter != mp->ttMove && mp->counter != mp->killer1
                && mp->counter != mp->killer2 && move_is_pseudo_legal(mp->board, mp->counter))
                return mp->counter;
            // Fallthrough

        case GEN_QUIET:
            // Generate and score all capture moves, except if the search tells
            // us to not do so due to quiet move pruning.
            ++mp->stage;
            if (!skipQuiets)
            {
                mp->list.last = generate_quiet(mp->cur, mp->board);
                score_quiet(mp, mp->cur, mp->list.last);
            }
            // Fallthrough

        case PICK_QUIET:
            // Stop selecting quiets if the search tells us to do so due to
            // quiet move pruning.
            if (!skipQuiets)
                while (mp->cur < mp->list.last)
                {
                    place_top_move(mp->cur, mp->list.last);
                    move_t move = (mp->cur++)->move;

                    // Return the move only if it is different from the TT move,
                    // the two killers and the countermove.
                    if (move != mp->ttMove && move != mp->killer1 && move != mp->killer2
                        && move != mp->counter)
                        return move;
                }

            ++mp->stage;
            mp->cur = mp->list.moves;
            // Fallthrough

        case PICK_BAD_INSTABLE:
            // Select all remaining captures. Note that we have already ordered
            // them at this point in the PICK_GOOD_INSTABLE phase.
            while (mp->cur < mp->badCaptures)
            {
                if (mp->cur->move != mp->ttMove) return (mp->cur++)->move;

                mp->cur++;
            }
            break;

        case CHECK_GEN_ALL:
            // Generate and score all evasions.
            ++mp->stage;
            mp->list.last = generate_evasions(mp->list.moves, mp->board);
            score_evasions(mp, mp->list.moves, mp->list.last);
            mp->cur = mp->list.moves;
            // Fallthrough

        case CHECK_PICK_ALL:
            // Select the next best evasion.
            while (mp->cur < mp->list.last)
            {
                place_top_move(mp->cur, mp->list.last);

                if (mp->cur->move != mp->ttMove) return (mp->cur++)->move;

                mp->cur++;
            }
            break;
    }

    // We went through all stages, so we can inform the search that there are no
    // moves left to pick.
    return NO_MOVE;
}
