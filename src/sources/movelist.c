/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

void place_top_move(extmove_t *begin, extmove_t *end)
{
    extmove_t tmp, *top = begin;

    for (extmove_t *i = begin + 1; i < end; ++i)
        if (i->score > top->score) top = i;

    tmp = *top;
    *top = *begin;
    *begin = tmp;
}

extmove_t *generate_piece_moves(
    extmove_t *movelist, const board_t *board, color_t us, piecetype_t pt, bitboard_t target)
{
    bitboard_t bb = piece_bb(board, us, pt);
    bitboard_t occupancy = occupancy_bb(board);

    while (bb)
    {
        square_t from = bb_pop_first_sq(&bb);
        bitboard_t b = piece_moves(pt, from, occupancy) & target;

        while (b) (movelist++)->move = create_move(from, bb_pop_first_sq(&b));
    }

    return (movelist);
}

extmove_t *generate_pawn_capture_moves(
    extmove_t *movelist, const board_t *board, color_t us, bitboard_t theirPieces, bool inQsearch)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank =
        piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);

    if (pawnsOnLastRank)
    {
        bitboard_t promote = relative_shift_up(pawnsOnLastRank, us);

        for (bitboard_t b = promote & empty; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush);
        }

        for (bitboard_t b = shift_left(promote) & theirPieces; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush - WEST, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush + WEST);
        }

        for (bitboard_t b = shift_right(promote) & theirPieces; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush - EAST, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush + EAST);
        }
    }

    bitboard_t capture = relative_shift_up(pawnsNotOnLastRank, us);

    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return (movelist);
}

extmove_t *generate_captures(extmove_t *movelist, const board_t *board, bool inQsearch)
{
    color_t us = board->sideToMove;
    bitboard_t target = color_bb(board, not_color(us));
    square_t kingSquare = get_king_square(board, us);

    movelist = generate_pawn_capture_moves(movelist, board, us, target, inQsearch);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    for (bitboard_t b = king_moves(kingSquare) & target; b;)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    return (movelist);
}

extmove_t *generate_quiet_pawn_moves(extmove_t *movelist, const board_t *board, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsNotOnLastRank =
        piece_bb(board, us, PAWN) & ~(us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 =
        relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    return (movelist);
}

extmove_t *generate_quiet(extmove_t *movelist, const board_t *board)
{
    color_t us = board->sideToMove;
    bitboard_t target = ~occupancy_bb(board);

    movelist = generate_quiet_pawn_moves(movelist, board, us);
    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    square_t kingSquare = get_king_square(board, us);
    bitboard_t b = king_moves(kingSquare) & target;

    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    int kingside = (us == WHITE) ? WHITE_OO : BLACK_OO;
    int queenside = (us == WHITE) ? WHITE_OOO : BLACK_OOO;

    if (!castling_blocked(board, kingside) && (board->stack->castlings & kingside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[kingside]);

    if (!castling_blocked(board, queenside) && (board->stack->castlings & queenside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[queenside]);

    return (movelist);
}

extmove_t *generate_classic_pawn_moves(extmove_t *movelist, const board_t *board, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank =
        piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t theirPieces = color_bb(board, not_color(us));
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 =
        relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    if (pawnsOnLastRank)
    {
        bitboard_t promote = relative_shift_up(pawnsOnLastRank, us);

        for (bitboard_t b = promote & empty; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush);

        for (bitboard_t b = shift_left(promote) & theirPieces; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush + WEST);

        for (bitboard_t b = shift_right(promote) & theirPieces; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush + EAST);
    }

    bitboard_t capture = relative_shift_up(pawnsNotOnLastRank, us);

    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return (movelist);
}

extmove_t *generate_classic(extmove_t *movelist, const board_t *board)
{
    color_t us = board->sideToMove;
    bitboard_t target = ~color_bb(board, us);

    movelist = generate_classic_pawn_moves(movelist, board, us);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    square_t kingSquare = get_king_square(board, us);
    bitboard_t b = king_moves(kingSquare) & target;

    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    int kingside = (us == WHITE) ? WHITE_OO : BLACK_OO;
    int queenside = (us == WHITE) ? WHITE_OOO : BLACK_OOO;

    if (!castling_blocked(board, kingside) && (board->stack->castlings & kingside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[kingside]);

    if (!castling_blocked(board, queenside) && (board->stack->castlings & queenside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[queenside]);

    return (movelist);
}

extmove_t *generate_pawn_evasion_moves(
    extmove_t *movelist, const board_t *board, bitboard_t blockSquares, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank =
        piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BITS : RANK_2_BITS);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t theirPieces = color_bb(board, not_color(us)) & blockSquares;
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 =
        relative_shift_up(push & (us == WHITE ? RANK_3_BITS : RANK_6_BITS), us) & empty;

    push &= blockSquares;
    push2 &= blockSquares;

    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    if (pawnsOnLastRank)
    {
        empty &= blockSquares;

        bitboard_t promote = relative_shift_up(pawnsOnLastRank, us);

        for (bitboard_t b = promote & empty; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush);

        for (bitboard_t b = shift_left(promote) & theirPieces; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush + WEST);

        for (bitboard_t b = shift_right(promote) & theirPieces; b;)
            movelist = create_promotions(movelist, bb_pop_first_sq(&b), pawnPush + EAST);
    }

    bitboard_t capture = relative_shift_up(pawnsNotOnLastRank, us);

    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    if (board->stack->enPassantSquare != SQ_NONE)
    {
        if (!(blockSquares & square_bb(board->stack->enPassantSquare - pawnPush)))
            return (movelist);

        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return (movelist);
}

extmove_t *generate_evasions(extmove_t *movelist, const board_t *board)
{
    color_t us = board->sideToMove;
    square_t kingSquare = get_king_square(board, us);
    bitboard_t sliderAttacks = 0;
    bitboard_t sliders = board->stack->checkers & ~piecetypes_bb(board, KNIGHT, PAWN);

    while (sliders)
    {
        square_t checkSquare = bb_pop_first_sq(&sliders);
        sliderAttacks |= LineBits[checkSquare][kingSquare] ^ square_bb(checkSquare);
    }

    bitboard_t b = king_moves(kingSquare) & ~color_bb(board, us) & ~sliderAttacks;

    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    // If in check in multiple times, we know only King moves can be legal.

    if (more_than_one(board->stack->checkers)) return (movelist);

    square_t checkSquare = bb_first_sq(board->stack->checkers);
    bitboard_t target = between_bb(checkSquare, kingSquare) | square_bb(checkSquare);

    movelist = generate_pawn_evasion_moves(movelist, board, target, us);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    return (movelist);
}

extmove_t *generate_all(extmove_t *movelist, const board_t *board)
{
    color_t us = board->sideToMove;
    bitboard_t pinned = board->stack->kingBlockers[us] & color_bb(board, us);
    square_t kingSquare = get_king_square(board, us);
    extmove_t *current = movelist;

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
