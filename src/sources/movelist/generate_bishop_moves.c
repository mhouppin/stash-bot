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

#include "movelist.h"

extmove_t   *generate_bishop_moves(extmove_t *movelist, const board_t *board,
            color_t us, bitboard_t target)
{
    bitboard_t  bb = piece_bb(board, us, BISHOP);

    while (bb)
    {
        square_t    from = pop_first_square(&bb);
        bitboard_t  b = bishop_moves(board, from) & target;

        while (b)
            (movelist++)->move = create_move(from, pop_first_square(&b));
    }

    return (movelist);
}
