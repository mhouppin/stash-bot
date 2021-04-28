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
#include <string.h>

INLINED extmove_t *create_promotions(extmove_t *movelist, square_t to, direction_t direction)
{
    (movelist++)->move = create_promotion(to - direction, to, QUEEN);
    (movelist++)->move = create_promotion(to - direction, to, ROOK);
    (movelist++)->move = create_promotion(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion(to - direction, to, KNIGHT);

    return (movelist);
}

INLINED extmove_t *create_underpromotions(extmove_t *movelist, square_t to, direction_t direction)
{
    (movelist++)->move = create_promotion(to - direction, to, ROOK);
    (movelist++)->move = create_promotion(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion(to - direction, to, KNIGHT);

    return (movelist);
}

void    place_top_move(extmove_t *begin, extmove_t *end)
{
    extmove_t   *top = begin;

    for (extmove_t *i = begin + 1; i < end; ++i)
        if (i->score > top->score)
            top = i;

    extmove_t   tmp = *top;

    *top = *begin;
    *begin = tmp;
}

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

extmove_t   *generate_pawn_capture_moves(extmove_t *movelist, const board_t *board, color_t us,
            bitboard_t their_pieces, bool inQsearch)
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
            if (!inQsearch)
                movelist = create_underpromotions(movelist, to, pawn_push);
        }

        for (bitboard_t b = shift_left(promote) & their_pieces; b; )
        {
            square_t    to = bb_pop_first_sq(&b);
            (movelist++)->move = create_promotion(to - pawn_push - WEST, to, QUEEN);
            if (!inQsearch)
                movelist = create_underpromotions(movelist, to, pawn_push + WEST);
        }

        for (bitboard_t b = shift_right(promote) & their_pieces; b; )
        {
            square_t    to = bb_pop_first_sq(&b);
            (movelist++)->move = create_promotion(to - pawn_push - EAST, to, QUEEN);
            if (!inQsearch)
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
    
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t  capture_en_passant = pawns_not_on_last_rank
            & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&capture_en_passant),
                board->stack->enPassantSquare);
    }

    return (movelist);
}

extmove_t   *generate_captures(extmove_t *movelist, const board_t *board, bool inQsearch)
{
    color_t     us = board->sideToMove;
    bitboard_t  target = color_bb(board, not_color(us));
    square_t    kingSquare = get_king_square(board, us);

    movelist = generate_pawn_capture_moves(movelist, board, us, target, inQsearch);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    for (bitboard_t b = king_moves(kingSquare) & target; b; )
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    return (movelist);
}

extmove_t   *generate_quiet_pawn_moves(extmove_t *movelist, const board_t *board, color_t us)
{
    int         pawn_push = pawn_direction(us);
    bitboard_t  pawns_not_on_last_rank = piece_bb(board, us, PAWN) & ~(us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t  empty = ~occupancy_bb(board);
    bitboard_t  push = relative_shift_up(pawns_not_on_last_rank, us) & empty;
    bitboard_t  push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

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

    return (movelist);
}

extmove_t   *generate_white_quiet(extmove_t *movelist, const board_t *board, bitboard_t target)
{
    movelist = generate_quiet_pawn_moves(movelist, board, WHITE);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, WHITE, pt, target);

    square_t    kingSquare = get_king_square(board, WHITE);
    bitboard_t  b = king_moves(kingSquare) & target;

    while (b)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    if (!castling_blocked(board, WHITE_OO) && (board->stack->castlings & WHITE_OO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[WHITE_OO]);

    if (!castling_blocked(board, WHITE_OOO) && (board->stack->castlings & WHITE_OOO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[WHITE_OOO]);

    return (movelist);
}

extmove_t   *generate_black_quiet(extmove_t *movelist, const board_t *board, bitboard_t target)
{
    movelist = generate_quiet_pawn_moves(movelist, board, BLACK);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, BLACK, pt, target);

    square_t    kingSquare = get_king_square(board, BLACK);
    bitboard_t  b = king_moves(kingSquare) & target;

    while (b)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    if (!castling_blocked(board, BLACK_OO) && (board->stack->castlings & BLACK_OO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[BLACK_OO]);

    if (!castling_blocked(board, BLACK_OOO) && (board->stack->castlings & BLACK_OOO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[BLACK_OOO]);

    return (movelist);
}

extmove_t   *generate_quiet(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->sideToMove;
    bitboard_t  target = ~occupancy_bb(board);

    return (us == WHITE ? generate_white_quiet(movelist, board, target)
        : generate_black_quiet(movelist, board, target));
}

extmove_t   *generate_classic_pawn_moves(extmove_t *movelist, const board_t *board, color_t us)
{
    int         pawn_push = pawn_direction(us);
    bitboard_t  pawns_on_last_rank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t  pawns_not_on_last_rank = piece_bb(board, us, PAWN) & ~pawns_on_last_rank;
    bitboard_t  empty = ~occupancy_bb(board);
    bitboard_t  their_pieces = color_bb(board, not_color(us));
    bitboard_t  push = relative_shift_up(pawns_not_on_last_rank, us) & empty;
    bitboard_t  push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

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
    
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t  capture_en_passant = pawns_not_on_last_rank
            & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&capture_en_passant),
                board->stack->enPassantSquare);
    }

    return (movelist);
}

extmove_t   *generate_white_classic(extmove_t *movelist, const board_t *board, bitboard_t target)
{
    movelist = generate_classic_pawn_moves(movelist, board, WHITE);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, WHITE, pt, target);

    square_t    kingSquare = get_king_square(board, WHITE);
    bitboard_t  b = king_moves(kingSquare) & target;

    while (b)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    if (!castling_blocked(board, WHITE_OO) && (board->stack->castlings & WHITE_OO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[WHITE_OO]);

    if (!castling_blocked(board, WHITE_OOO) && (board->stack->castlings & WHITE_OOO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[WHITE_OOO]);

    return (movelist);
}

extmove_t   *generate_black_classic(extmove_t *movelist, const board_t *board, bitboard_t target)
{
    movelist = generate_classic_pawn_moves(movelist, board, BLACK);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, BLACK, pt, target);

    square_t    kingSquare = get_king_square(board, BLACK);
    bitboard_t  b = king_moves(kingSquare) & target;

    while (b)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    if (!castling_blocked(board, BLACK_OO) && (board->stack->castlings & BLACK_OO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[BLACK_OO]);

    if (!castling_blocked(board, BLACK_OOO) && (board->stack->castlings & BLACK_OOO))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[BLACK_OOO]);

    return (movelist);
}

extmove_t   *generate_classic(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->sideToMove;
    bitboard_t  target = ~color_bb(board, us);

    return (us == WHITE ? generate_white_classic(movelist, board, target)
        : generate_black_classic(movelist, board, target));
}

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

    if (board->stack->enPassantSquare != SQ_NONE)
    {
        if (!(block_squares & square_bb(board->stack->enPassantSquare - pawn_push)))
            return (movelist);

        bitboard_t  capture_en_passant = pawns_not_on_last_rank
            & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (capture_en_passant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&capture_en_passant),
                board->stack->enPassantSquare);
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
    color_t     us = board->sideToMove;
    square_t    kingSquare = get_king_square(board, us);
    bitboard_t  slider_attacks = 0;
    bitboard_t  sliders = board->stack->checkers & ~piecetypes_bb(board, KNIGHT, PAWN);

    while (sliders)
    {
        square_t    check_square = bb_pop_first_sq(&sliders);
        slider_attacks |= LineBits[check_square][kingSquare] ^ square_bb(check_square);
    }

    bitboard_t  b = king_moves(kingSquare) & ~color_bb(board, us) & ~slider_attacks;

    while (b)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    if (more_than_one(board->stack->checkers))
        return (movelist);

    square_t    check_square = bb_first_sq(board->stack->checkers);
    bitboard_t  target = between_bb(check_square, kingSquare) | square_bb(check_square);

    if (us == WHITE)
        return (generate_white_evasions(movelist, board, target));
    else
        return (generate_black_evasions(movelist, board, target));
}

extmove_t   *generate_all(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->sideToMove;
    bitboard_t  pinned = board->stack->kingBlockers[us] & color_bb(board, us);
    square_t    kingSquare = get_king_square(board, us);
    extmove_t   *current = movelist;

    movelist = board->stack->checkers ? generate_evasions(movelist, board)
        : generate_classic(movelist, board);

    while (current < movelist)
    {
        if ((pinned || from_sq(current->move) == kingSquare
            || move_type(current->move) == EN_PASSANT)
            && !move_is_legal(board, current->move))
            current->move = (--movelist)->move;
        else
            ++current;
    }

    return (movelist);
}
