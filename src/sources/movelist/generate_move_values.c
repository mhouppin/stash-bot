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

#include "history.h"
#include "lazy_smp.h"
#include "movelist.h"

void    generate_move_values(movelist_t *movelist, const board_t *board,
        move_t tt_move, move_t *killers, move_t previous_move)
{
    worker_t *const     worker = get_worker(board);
    extmove_t *const    end = movelist->last;
    move_t              counter;
    square_t            lto = SQ_A1;
    piece_t             lpc = NO_PIECE;
    square_t            from, to;
    piece_t             moved_piece, captured_piece;

    if (is_valid_move(previous_move))
    {
        lto = to_sq(previous_move);
        lpc = piece_on(board, lto);

        counter = worker->cm_history[lpc][lto];
    }
    else
        counter = NO_MOVE;

    for (extmove_t *extmove = movelist->moves; extmove < end; ++extmove)
    {
        move_t  move = extmove->move;

        if (move == tt_move)
        {
            extmove->score = 16384;
            continue ;
        }

        switch (move_type(move))
        {
            case PROMOTION:
                extmove->score = promotion_type(move) == QUEEN ? 8192 : -4096;
                break ;

            case EN_PASSANT:
                extmove->score = 4096 + PAWN * 8 - PAWN;
                break ;

            default:
                from = from_sq(move);
                to = to_sq(move);
                moved_piece = piece_on(board, from);
                captured_piece = piece_on(board, to);

                if (captured_piece != NO_PIECE)
                {
                    extmove->score = see_greater_than(board, move, -30) ? 4096 : 2048;
                    extmove->score += piece_type(captured_piece) * 8;
                    extmove->score -= piece_type(moved_piece);
                }
                else if (move == killers[0] || move == killers[1])
                    extmove->score = 3073;

                else if (move == counter)
                    extmove->score = 3072;

                else
                    extmove->score = get_bf_history_score(worker->bf_history, moved_piece, move)
                        + get_ct_history_score(worker->ct_history, moved_piece, to, lpc, lto);
                break ;
        }
    }
}
