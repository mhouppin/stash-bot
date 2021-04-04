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

void    set_boardstack(board_t *board, boardstack_t *stack)
{
    stack->board_key = stack->pawn_key = board->stack->material_key = 0;
    stack->material[WHITE] = stack->material[BLACK] = 0;
    stack->checkers = attackers_to(board, get_king_square(board, board->side_to_move))
        & color_bb(board, not_color(board->side_to_move));

    set_check(board, stack);

    for (bitboard_t b = board->piecetype_bits[ALL_PIECES]; b; )
    {
        square_t    square = bb_pop_first_sq(&b);
        piece_t     piece = piece_on(board, square);

        stack->board_key ^= ZobristPsq[piece][square];

        if (piece_type(piece) == PAWN)
            stack->pawn_key ^= ZobristPsq[piece][square];

        else if (piece_type(piece) != KING)
            stack->material[piece_color(piece)] += PieceScores[MIDGAME][piece];
    }

    if (stack->en_passant_square != SQ_NONE)
        stack->board_key ^= ZobristEnPassant[sq_file(stack->en_passant_square)];

    if (board->side_to_move == BLACK)
        stack->board_key ^= ZobristBlackToMove;

    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt)
        {
            piece_t     pc = create_piece(c, pt);

            for (int i = 0; i < board->piece_count[pc]; ++i)
                stack->material_key ^= ZobristPsq[pc][i];
        }

    stack->board_key ^= ZobristCastling[stack->castlings];
}
