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


#include "board.h"

void    do_castling(board_t *board, color_t us, square_t king_from,
        square_t *king_to, square_t *rook_from, square_t *rook_to)
{
    bool    kingside = *king_to > king_from;

    *rook_from = *king_to;
    *rook_to = relative_square(kingside ? SQ_F1 : SQ_D1, us);
    *king_to = relative_square(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, king_from);
    remove_piece(board, *rook_from);
    board->table[king_from] = board->table[*rook_from] = NO_PIECE;
    put_piece(board, create_piece(us, KING), *king_to);
    put_piece(board, create_piece(us, ROOK), *rook_to);
}

void    undo_castling(board_t *board, color_t us, square_t king_from,
        square_t *king_to, square_t *rook_from, square_t *rook_to)
{
    bool    kingside = *king_to > king_from;

    *rook_from = *king_to;
    *rook_to = relative_square(kingside ? SQ_F1 : SQ_D1, us);
    *king_to = relative_square(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, *king_to);
    remove_piece(board, *rook_to);
    board->table[*king_to] = board->table[*rook_to] = NO_PIECE;
    put_piece(board, create_piece(us, KING), king_from);
    put_piece(board, create_piece(us, ROOK), *rook_from);
}
