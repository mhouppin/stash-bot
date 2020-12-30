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

extmove_t   *generate_capture_white_pawn_moves(extmove_t *movelist,
            const board_t *board)
{
    bitboard_t  pawns_on_rank_7 = piece_bb(board, WHITE, PAWN) & RANK_7_BITS;
    bitboard_t  pawns_not_on_rank_7 = piece_bb(board, WHITE, PAWN) & ~RANK_7_BITS;
    bitboard_t  enemies = board->color_bits[BLACK];
    bitboard_t  empty_squares = ~board->piecetype_bits[ALL_PIECES];

    if (pawns_on_rank_7)
    {
        bitboard_t  promote = shift_up(pawns_on_rank_7) & empty_squares;
        bitboard_t  promote_left = shift_up_left(pawns_on_rank_7) & enemies;
        bitboard_t  promote_right = shift_up_right(pawns_on_rank_7) & enemies;

        while (promote)
        {
            square_t    to = pop_first_square(&promote);
            (movelist++)->move = create_promotion(to - NORTH, to, QUEEN);
        }

        while (promote_left)
        {
            square_t    to = pop_first_square(&promote_left);
            (movelist++)->move = create_promotion(to - NORTH_WEST, to, QUEEN);
        }

        while (promote_right)
        {
            square_t    to = pop_first_square(&promote_right);
            (movelist++)->move = create_promotion(to - NORTH_EAST, to, QUEEN);
        }
    }

    bitboard_t  capture_left = shift_up_left(pawns_not_on_rank_7) & enemies;
    bitboard_t  capture_right = shift_up_right(pawns_not_on_rank_7) & enemies;

    while (capture_left)
    {
        square_t    to = pop_first_square(&capture_left);
        (movelist++)->move = create_move(to - NORTH_WEST, to);
    }

    while (capture_right)
    {
        square_t    to = pop_first_square(&capture_right);
        (movelist++)->move = create_move(to - NORTH_EAST, to);
    }

    if (board->stack->en_passant_square != SQ_NONE)
    {
        bitboard_t  capture_en_passant = pawns_not_on_rank_7
            & pawn_moves(board->stack->en_passant_square, BLACK);

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                pop_first_square(&capture_en_passant),
                board->stack->en_passant_square);
    }

    return (movelist);
}

extmove_t   *generate_capture_black_pawn_moves(extmove_t *movelist,
            const board_t *board)
{
    bitboard_t  pawns_on_rank_7 = piece_bb(board, BLACK, PAWN) & RANK_2_BITS;
    bitboard_t  pawns_not_on_rank_7 = piece_bb(board, BLACK, PAWN) & ~RANK_2_BITS;
    bitboard_t  enemies = board->color_bits[WHITE];
    bitboard_t  empty_squares = ~board->piecetype_bits[ALL_PIECES];

    if (pawns_on_rank_7)
    {
        bitboard_t  promote = shift_down(pawns_on_rank_7) & empty_squares;
        bitboard_t  promote_left = shift_down_left(pawns_on_rank_7) & enemies;
        bitboard_t  promote_right = shift_down_right(pawns_on_rank_7) & enemies;

        while (promote)
        {
            square_t    to = pop_first_square(&promote);
            (movelist++)->move = create_promotion(to - SOUTH, to, QUEEN);
        }

        while (promote_left)
        {
            square_t    to = pop_first_square(&promote_left);
            (movelist++)->move = create_promotion(to - SOUTH_WEST, to, QUEEN);
        }

        while (promote_right)
        {
            square_t    to = pop_first_square(&promote_right);
            (movelist++)->move = create_promotion(to - SOUTH_EAST, to, QUEEN);
        }
    }

    bitboard_t  capture_left = shift_down_left(pawns_not_on_rank_7) & enemies;
    bitboard_t  capture_right = shift_down_right(pawns_not_on_rank_7) & enemies;

    while (capture_left)
    {
        square_t    to = pop_first_square(&capture_left);
        (movelist++)->move = create_move(to - SOUTH_WEST, to);
    }

    while (capture_right)
    {
        square_t    to = pop_first_square(&capture_right);
        (movelist++)->move = create_move(to - SOUTH_EAST, to);
    }

    if (board->stack->en_passant_square != SQ_NONE)
    {
        bitboard_t  capture_en_passant = pawns_not_on_rank_7
            & pawn_moves(board->stack->en_passant_square, WHITE);

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                pop_first_square(&capture_en_passant),
                board->stack->en_passant_square);
    }

    return (movelist);
}

extmove_t   *generate_white_captures(extmove_t *movelist, const board_t *board,
            bitboard_t target)
{
    movelist = generate_capture_white_pawn_moves(movelist, board);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, WHITE, pt, target);

    square_t    king_square = board_king_square(board, WHITE);
    bitboard_t  b = king_moves(king_square) & target;

    while (b)
        (movelist++)->move = create_move(king_square, pop_first_square(&b));

    return (movelist);
}

extmove_t   *generate_black_captures(extmove_t *movelist, const board_t *board,
            bitboard_t target)
{
    movelist = generate_capture_black_pawn_moves(movelist, board);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, BLACK, pt, target);

    square_t    king_square = board_king_square(board, BLACK);
    bitboard_t  b = king_moves(king_square) & target;

    while (b)
        (movelist++)->move = create_move(king_square, pop_first_square(&b));

    return (movelist);
}

extmove_t   *generate_captures(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->side_to_move;
    bitboard_t  target = board->color_bits[not_color(us)];

    return (us == WHITE ? generate_white_captures(movelist, board, target)
        : generate_black_captures(movelist, board, target));
}
