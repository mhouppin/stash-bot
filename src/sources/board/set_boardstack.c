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

#include "board.h"

void    set_boardstack(board_t *board, boardstack_t *stack)
{
    stack->board_key = stack->pawn_key = 0;
    stack->checkers = attackers_to(board, board_king_square(board, board->side_to_move))
        & board->color_bits[not_color(board->side_to_move)];

    set_check(board, stack);

    for (bitboard_t b = board->piecetype_bits[ALL_PIECES]; b; )
    {
        square_t    square = pop_first_square(&b);
        piece_t     piece = piece_on(board, square);

        stack->board_key ^= ZobristPsq[piece][square];

        if (type_of_piece(piece) == PAWN)
            stack->pawn_key ^= ZobristPsq[piece][square];
    }

    if (stack->en_passant_square != SQ_NONE)
        stack->board_key ^= ZobristEnPassant[
            file_of_square(stack->en_passant_square)];

    if (board->side_to_move == BLACK)
        stack->board_key ^= ZobristBlackToMove;

    stack->board_key ^= ZobristCastling[stack->castlings];
}
