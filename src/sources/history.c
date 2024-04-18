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

#include <math.h>
#include "history.h"
#include "search.h"
#include "worker.h"

double HistQuad = 24.0;
double HistLinear = 1.0;
double HistBase = 0.0;
double HistMax = 2563.0;

int history_bonus(int depth)
{
    return fmin(HistMax, HistQuad * depth * depth + HistLinear * depth + HistBase);
}

void update_cont_histories(Searchstack *ss, int depth, piece_t piece, square_t to, bool failHigh)
{
    int bonus = history_bonus(depth);

    if (!failHigh) bonus = -bonus;

    if ((ss - 1)->pieceHistory != NULL) add_pc_history(*(ss - 1)->pieceHistory, piece, to, bonus);
    if ((ss - 2)->pieceHistory != NULL) add_pc_history(*(ss - 2)->pieceHistory, piece, to, bonus);
    if ((ss - 4)->pieceHistory != NULL) add_pc_history(*(ss - 4)->pieceHistory, piece, to, bonus);
}

void update_quiet_history(const Board *board, int depth, move_t bestmove, const move_t quiets[64],
    int qcount, Searchstack *ss)
{
    butterfly_history_t *bfHist = &get_worker(board)->bfHistory;
    const int bonus = history_bonus(depth);
    const move_t previousMove = (ss - 1)->currentMove;
    square_t to;
    piece_t piece;

    piece = piece_on(board, from_sq(bestmove));
    to = to_sq(bestmove);

    // Apply history bonuses to the bestmove.
    if ((ss - 1)->pieceHistory != NULL)
    {
        const square_t lastTo = to_sq(previousMove);
        const piece_t lastPiece = piece_on(board, lastTo);

        get_worker(board)->cmHistory[lastPiece][lastTo] = bestmove;
    }

    add_bf_history(*bfHist, piece, bestmove, bonus);
    update_cont_histories(ss, depth, piece, to, true);

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
        update_cont_histories(ss, depth, piece, to, false);
    }
}

void update_single_capture(capture_history_t *capHist, const Board *board, move_t move, int bonus)
{
    const piece_t movedPiece = piece_on(board, from_sq(move));
    const square_t to = to_sq(move);
    piecetype_t captured = piece_type(piece_on(board, to));

    if (move_type(move) == PROMOTION)
        captured = piece_type(promotion_type(move));
    else if (move_type(move) == EN_PASSANT)
        captured = PAWN;

    add_cap_history(*capHist, movedPiece, to, captured, bonus);
}

void update_capture_history(const Board *board, int depth, move_t bestmove,
    const move_t captures[64], int ccount, Searchstack *ss)
{
    (void)ss;
    capture_history_t *capHist = &get_worker(board)->capHistory;
    const int bonus = history_bonus(depth);

    // Apply history bonuses to the bestmove.
    if (is_capture_or_promotion(board, bestmove))
        update_single_capture(capHist, board, bestmove, bonus);

    // Apply history penalties to all previous failing capture moves.
    for (int i = 0; i < ccount; ++i) update_single_capture(capHist, board, captures[i], -bonus);
}
