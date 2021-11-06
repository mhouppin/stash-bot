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

#include "movepick.h"

void movepick_init(movepick_t *mp, bool inQsearch, const board_t *board,
    const worker_t *worker, move_t ttMove, searchstack_t *ss)
{
    mp->inQsearch = inQsearch;

    if (board->stack->checkers)
        mp->stage = CHECK_PICK_TT + !(ttMove && move_is_pseudo_legal(board, ttMove));

    else
    {
        mp->stage = PICK_TT + !(ttMove && (!inQsearch || is_capture_or_promotion(board, ttMove))
            && move_is_pseudo_legal(board, ttMove));
    }

    mp->ttMove = ttMove;
    mp->killer1 = ss->killers[0];
    mp->killer2 = ss->killers[1];

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

static void score_captures(movepick_t *mp, extmove_t *begin, extmove_t *end)
{
    static const score_t MVV_LVA[PIECETYPE_NB] = {
        0, 0, 640, 640, 1280, 2560, 0, 0
    };

    while (begin < end)
    {
        move_t move = begin->move;
        square_t to = to_sq(move);
        piece_t movedPiece = piece_on(mp->board, from_sq(move));
        piecetype_t captured = piece_type(piece_on(mp->board, to));

        if (move_type(move) == PROMOTION)
        {
            begin->score = MVV_LVA[promotion_type(move)];
            captured = piece_type(promotion_type(move));
        }
        else if (move_type(move) == EN_PASSANT)
        {
            begin->score = MVV_LVA[PAWN];
            captured = PAWN;
        }
        else
            begin->score = MVV_LVA[captured];

        begin->score += get_cap_history_score(mp->worker->capHistory, movedPiece, to, captured);

        ++begin;
    }
}

static void score_quiet(movepick_t *mp, extmove_t *begin, extmove_t *end)
{
    while (begin < end)
    {
        piece_t moved = piece_on(mp->board, from_sq(begin->move));
        square_t to = to_sq(begin->move);

        begin->score = get_bf_history_score(mp->worker->bfHistory, moved, begin->move);

        if (mp->pieceHistory[0] != NULL)
            begin->score += get_pc_history_score(*mp->pieceHistory[0], moved, to);
        if (mp->pieceHistory[1] != NULL)
            begin->score += get_pc_history_score(*mp->pieceHistory[1], moved, to);

        ++begin;
    }
}

static void score_evasions(movepick_t *mp, extmove_t *begin, extmove_t *end)
{
    while (begin < end)
    {
        if (is_capture_or_promotion(mp->board, begin->move))
        {
            piecetype_t moved = piece_type(piece_on(mp->board, from_sq(begin->move)));
            piecetype_t captured = piece_type(piece_on(mp->board, to_sq(begin->move)));

            begin->score = 28672 + captured * 8 - moved;
        }
        else
        {
            piece_t moved = piece_on(mp->board, from_sq(begin->move));
            square_t to = to_sq(begin->move);

            begin->score = get_bf_history_score(mp->worker->bfHistory, moved, begin->move);

            if (mp->pieceHistory[0] != NULL)
                begin->score += get_pc_history_score(*mp->pieceHistory[0], moved, to);
            if (mp->pieceHistory[1] != NULL)
                begin->score += get_pc_history_score(*mp->pieceHistory[1], moved, to);
        }

        ++begin;
    }
}

move_t movepick_next_move(movepick_t *mp, bool skipQuiets)
{
__top:

    switch (mp->stage)
    {
        case PICK_TT:
        case CHECK_PICK_TT:
            ++mp->stage;
            return (mp->ttMove);

        case GEN_INSTABLE:
            ++mp->stage;
            mp->list.last = generate_captures(mp->list.moves, mp->board, mp->inQsearch);
            score_captures(mp, mp->list.moves, mp->list.last);
            mp->cur = mp->badCaptures = mp->list.moves;
            // Fallthrough

        case PICK_GOOD_INSTABLE:
            while (mp->cur < mp->list.last)
            {
                place_top_move(mp->cur, mp->list.last);

                if (mp->cur->move != mp->ttMove && see_greater_than(mp->board, mp->cur->move, 0))
                    return ((mp->cur++)->move);

                *(mp->badCaptures++) = *(mp->cur++);
            }

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
            if (mp->killer1
                && mp->killer1 != mp->ttMove
                && move_is_pseudo_legal(mp->board, mp->killer1))
                return (mp->killer1);
            // Fallthrough

        case PICK_KILLER2:
            ++mp->stage;
            if (mp->killer2
                && mp->killer2 != mp->ttMove
                && mp->killer2 != mp->killer1
                && move_is_pseudo_legal(mp->board, mp->killer2))
                return (mp->killer2);
            // Fallthrough

        case PICK_COUNTER:
            ++mp->stage;
            if (mp->counter
                && mp->counter != mp->ttMove
                && mp->counter != mp->killer1
                && mp->counter != mp->killer2
                && move_is_pseudo_legal(mp->board, mp->counter))
                return (mp->counter);
            // Fallthrough

        case GEN_QUIET:
            ++mp->stage;
            if (!skipQuiets)
            {
                mp->list.last = generate_quiet(mp->cur, mp->board);
                score_quiet(mp, mp->cur, mp->list.last);
            }
            // Fallthrough

        case PICK_QUIET:
            if (!skipQuiets)
                while (mp->cur < mp->list.last)
                {
                    place_top_move(mp->cur, mp->list.last);
                    move_t move = (mp->cur++)->move;

                    if (move != mp->ttMove
                        && move != mp->killer1
                        && move != mp->killer2
                        && move != mp->counter)
                        return (move);
                }

            ++mp->stage;
            mp->cur = mp->list.moves;
            // Fallthrough

        case PICK_BAD_INSTABLE:
            while (mp->cur < mp->badCaptures)
            {
                if (mp->cur->move != mp->ttMove)
                    return ((mp->cur++)->move);

                mp->cur++;
            }
            break ;

        case CHECK_GEN_ALL:
            ++mp->stage;
            mp->list.last = generate_evasions(mp->list.moves, mp->board);
            score_evasions(mp, mp->list.moves, mp->list.last);
            mp->cur = mp->list.moves;
            // Fallthrough

        case CHECK_PICK_ALL:
            while (mp->cur < mp->list.last)
            {
                place_top_move(mp->cur, mp->list.last);

                if (mp->cur->move != mp->ttMove)
                    return ((mp->cur++)->move);

                mp->cur++;
            }
            break ;
    }
    return (NO_MOVE);
}
