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

#include "engine.h"
#include "lazy_smp.h"

void update_quiet_history(const board_t *board, int depth,
    move_t bestmove, const move_t quiets[64], int qcount, searchstack_t *ss)
{
    butterfly_history_t *bfHist = &get_worker(board)->bfHistory;
    square_t lastTo = SQ_A1;
    piece_t lastPiece = NO_PIECE;
    square_t to;
    piece_t piece;
    int bonus = history_bonus(depth);
    move_t previousMove = (ss - 1)->currentMove;

    piece = piece_on(board, from_sq(bestmove));
    to = to_sq(bestmove);

    // Apply history bonuses to the bestmove.

    if ((ss - 1)->pieceHistory != NULL)
    {
        lastTo = to_sq(previousMove);
        lastPiece = piece_on(board, lastTo);

        get_worker(board)->cmHistory[lastPiece][lastTo] = bestmove;
        add_pc_history(*(ss - 1)->pieceHistory, piece, to, bonus);
    }
    if ((ss - 2)->pieceHistory != NULL)
        add_pc_history(*(ss - 2)->pieceHistory, piece, to, bonus);

    add_bf_history(*bfHist, piece, bestmove, bonus);

    // Set the bestmove as a killer.

    if (ss->killers[0] != bestmove)
    {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestmove;
    }

    // Apply history penalties to all previous failing quiet moves.

    for (int i = 0; i < qcount; ++i)
    {
        piece = piece_on(board, from_sq(quiets[i]));
        to = to_sq(quiets[i]);
        add_bf_history(*bfHist, piece, quiets[i], -bonus);

        if ((ss - 1)->pieceHistory != NULL)
            add_pc_history(*(ss - 1)->pieceHistory, piece, to, -bonus);
        if ((ss - 2)->pieceHistory != NULL)
            add_pc_history(*(ss - 2)->pieceHistory, piece, to, -bonus);
    }
}

void update_single_capture(capture_history_t *capHist, const board_t *board,
    move_t move, int bonus)
{
    square_t from = from_sq(move);
    piece_t movedPiece = piece_on(board, from);
    square_t to = to_sq(move);
    piecetype_t captured = piece_type(piece_on(board, to));

    if (move_type(move) == PROMOTION)
        captured = piece_type(promotion_type(move));
    else if (move_type(move) == EN_PASSANT)
        captured = PAWN;

    add_cap_history(*capHist, movedPiece, to, captured, bonus);
}

void update_capture_history(const board_t *board, int depth,
    move_t bestmove, const move_t captures[64], int ccount, searchstack_t *ss)
{
    (void)ss;
    capture_history_t *capHist = &get_worker(board)->capHistory;
    int bonus = history_bonus(depth);

    update_single_capture(capHist, board, bestmove, bonus);

    for (int i = 0; i < ccount; ++i)
        update_single_capture(capHist, board, captures[i], -bonus);
}
