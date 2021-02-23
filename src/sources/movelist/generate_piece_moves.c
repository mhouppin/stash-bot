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

#include "movelist.h"

extmove_t   *generate_piece_moves(extmove_t *movelist, const board_t *board,
            color_t us, piecetype_t pt, bitboard_t target)
{
    bitboard_t  bb = piece_bb(board, us, pt);
    bitboard_t  occupancy = occupancy_bb(board);

    while (bb)
    {
        square_t    from = bb_pop_first_sq(&bb);
        bitboard_t  b = piece_moves(pt, from, occupancy) & target;

        while (b)
            (movelist++)->move = create_move(from, bb_pop_first_sq(&b));
    }

    return (movelist);
}
