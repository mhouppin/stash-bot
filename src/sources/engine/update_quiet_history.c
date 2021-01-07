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

#include "engine.h"
#include "lazy_smp.h"

void    update_quiet_history(const board_t *board, int depth,
        move_t bestmove, const move_t quiets[64], int qcount, searchstack_t *ss)
{
    bf_history_t    *bf_hist = &get_worker(board)->bf_history;
    ct_history_t    *ct_hist = &get_worker(board)->ct_history;
    square_t        lto = SQ_A1;
    piece_t         lpc = NO_PIECE;
    square_t        to;
    piece_t         pc;
    int             bonus = (depth <= 12) ? 16 * depth * depth : 20;
    move_t          previous_move = (ss - 1)->current_move;

    if (is_valid_move(previous_move))
    {
        lto = move_to_square(previous_move);
        lpc = piece_on(board, lto);

        get_worker(board)->cm_history[lpc][lto] = bestmove;
    }

    pc = piece_on(board, move_from_square(bestmove));
    to = move_to_square(bestmove);

    add_bf_history(*bf_hist, pc, bestmove, bonus);
    add_ct_history(*ct_hist, pc, to, lpc, lto, bonus);

    if (ss->killers[0] == NO_MOVE)
        ss->killers[0] = bestmove;
    else if (ss->killers[0] != bestmove)
        ss->killers[1] = bestmove;

    for (int i = 0; i < qcount; ++i)
    {
        pc = piece_on(board, move_from_square(quiets[i]));
        to = move_to_square(quiets[i]);
        add_bf_history(*bf_hist, pc, quiets[i], -bonus);
        add_ct_history(*ct_hist, pc, to, lpc, lto, -bonus);
    }
}
