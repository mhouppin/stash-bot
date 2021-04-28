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

#include "engine.h"
#include "lazy_smp.h"

void    update_quiet_history(const board_t *board, int depth,
        move_t bestmove, const move_t quiets[64], int qcount, searchstack_t *ss)
{
    butterfly_history_t    *bf_hist = &get_worker(board)->bfHistory;
    square_t        lto = SQ_A1;
    piece_t         lpc = NO_PIECE;
    square_t        to;
    piece_t         pc;
    int             bonus = (depth <= 12) ? 32 * depth * depth : 40;
    move_t          previous_move = (ss - 1)->currentMove;

    pc = piece_on(board, from_sq(bestmove));
    to = to_sq(bestmove);

    if ((ss - 1)->pieceHistory != NULL)
    {
        lto = to_sq(previous_move);
        lpc = piece_on(board, lto);

        get_worker(board)->cmHistory[lpc][lto] = bestmove;
        add_pc_history(*(ss - 1)->pieceHistory, pc, to, bonus);
    }
    if ((ss - 2)->pieceHistory != NULL)
        add_pc_history(*(ss - 2)->pieceHistory, pc, to, bonus);

    add_bf_history(*bf_hist, pc, bestmove, bonus);

    if (ss->killers[0] != bestmove)
    {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestmove;
    }

    for (int i = 0; i < qcount; ++i)
    {
        pc = piece_on(board, from_sq(quiets[i]));
        to = to_sq(quiets[i]);
        add_bf_history(*bf_hist, pc, quiets[i], -bonus);

        if ((ss - 1)->pieceHistory != NULL)
            add_pc_history(*(ss - 1)->pieceHistory, pc, to, -bonus);
        if ((ss - 2)->pieceHistory != NULL)
            add_pc_history(*(ss - 2)->pieceHistory, pc, to, -bonus);
    }
}
