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

extmove_t   *generate_pawn_evasion_moves(extmove_t *movelist, const board_t *board,
            bitboard_t block_squares, color_t us)
{
    int         pawn_push = pawn_direction(us);
    bitboard_t  pawns_on_last_rank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t  pawns_not_on_last_rank = piece_bb(board, us, PAWN) & ~pawns_on_last_rank;
    bitboard_t  empty = ~occupancy_bb(board);
    bitboard_t  their_pieces = color_bb(board, not_color(us)) & block_squares;
    bitboard_t  push = relative_shift_up(pawns_not_on_last_rank, us) & empty;
    bitboard_t  push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

    push &= block_squares;
    push2 &= block_squares;

    while (push)
    {
        square_t    to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawn_push, to);
    }

    while (push2)
    {
        square_t    to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawn_push - pawn_push, to);
    }

    if (pawns_on_last_rank)
    {
        empty &= block_squares;

        bitboard_t  promote = relative_shift_up(pawns_on_last_rank, us);

        for (bitboard_t b = promote & empty; b; )
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawn_push);

        for (bitboard_t b = shift_left(promote) & their_pieces; b; )
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawn_push + WEST);

        for (bitboard_t b = shift_right(promote) & their_pieces; b; )
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawn_push + EAST);
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
        if (!(block_squares & square_bb(board->stack->en_passant_square - pawn_push)))
            return (movelist);

        bitboard_t  capture_en_passant = pawns_not_on_last_rank
            & pawn_moves(board->stack->en_passant_square, not_color(us));

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&capture_en_passant),
                board->stack->en_passant_square);
    }

    return (movelist);
}

extmove_t   *generate_white_evasions(extmove_t *movelist, const board_t *board, bitboard_t block_squares)
{
    movelist = generate_pawn_evasion_moves(movelist, board, block_squares, WHITE);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, WHITE, pt, block_squares);

    return (movelist);
}

extmove_t   *generate_black_evasions(extmove_t *movelist, const board_t *board, bitboard_t block_squares)
{
    movelist = generate_pawn_evasion_moves(movelist, board, block_squares, BLACK);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, BLACK, pt, block_squares);

    return (movelist);
}

extmove_t   *generate_evasions(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->side_to_move;
    square_t    king_square = get_king_square(board, us);
    bitboard_t  slider_attacks = 0;
    bitboard_t  sliders = board->stack->checkers & ~piecetypes_bb(board, KNIGHT, PAWN);

    while (sliders)
    {
        square_t    check_square = bb_pop_first_sq(&sliders);
        slider_attacks |= LineBits[check_square][king_square] ^ square_bb(check_square);
    }

    bitboard_t  b = king_moves(king_square) & ~color_bb(board, us) & ~slider_attacks;

    while (b)
        (movelist++)->move = create_move(king_square, bb_pop_first_sq(&b));

    if (more_than_one(board->stack->checkers))
        return (movelist);

    square_t    check_square = bb_first_sq(board->stack->checkers);
    bitboard_t  target = between_bb(check_square, king_square) | square_bb(check_square);

    if (us == WHITE)
        return (generate_white_evasions(movelist, board, target));
    else
        return (generate_black_evasions(movelist, board, target));
}
