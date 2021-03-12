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

extmove_t   *generate_pawn_capture_moves(extmove_t *movelist, const board_t *board, color_t us,
            bitboard_t their_pieces, bool in_qsearch)
{
    int         pawn_push = pawn_direction(us);
    bitboard_t  pawns_on_last_rank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t  pawns_not_on_last_rank = piece_bb(board, us, PAWN) & ~pawns_on_last_rank;
    bitboard_t  empty = ~occupancy_bb(board);

    if (pawns_on_last_rank)
    {
        bitboard_t  promote = relative_shift_up(pawns_on_last_rank, us);

        for (bitboard_t b = promote & empty; b; )
        {
            square_t    to = bb_pop_first_sq(&b);
            (movelist++)->move = create_promotion(to - pawn_push, to, QUEEN);
            if (!in_qsearch)
                movelist = create_underpromotions(movelist, to, pawn_push);
        }

        for (bitboard_t b = shift_left(promote) & their_pieces; b; )
        {
            square_t    to = bb_pop_first_sq(&b);
            (movelist++)->move = create_promotion(to - pawn_push - WEST, to, QUEEN);
            if (!in_qsearch)
                movelist = create_underpromotions(movelist, to, pawn_push + WEST);
        }

        for (bitboard_t b = shift_right(promote) & their_pieces; b; )
        {
            square_t    to = bb_pop_first_sq(&b);
            (movelist++)->move = create_promotion(to - pawn_push - EAST, to, QUEEN);
            if (!in_qsearch)
                movelist = create_underpromotions(movelist, to, pawn_push + EAST);
        }
    }

    bitboard_t  capture = relative_shift_up(pawns_not_on_last_rank, us);

    for (bitboard_t b = shift_left(capture) & their_pieces; b; )
    {
        square_t    to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawn_push - WEST, to);
    }

    for (bitboard_t b = shift_right(capture) & their_pieces; b; )
    {
        square_t    to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawn_push - EAST, to);
    }
    
    if (board->stack->en_passant_square != SQ_NONE)
    {
        bitboard_t  capture_en_passant = pawns_not_on_last_rank
            & pawn_moves(board->stack->en_passant_square, not_color(us));

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&capture_en_passant),
                board->stack->en_passant_square);
    }

    return (movelist);
}

extmove_t   *generate_captures(extmove_t *movelist, const board_t *board, bool in_qsearch)
{
    color_t     us = board->side_to_move;
    bitboard_t  target = color_bb(board, not_color(us));
    square_t    king_square = get_king_square(board, us);

    movelist = generate_pawn_capture_moves(movelist, board, us, target, in_qsearch);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    for (bitboard_t b = king_moves(king_square) & target; b; )
        (movelist++)->move = create_move(king_square, bb_pop_first_sq(&b));

    return (movelist);
}
