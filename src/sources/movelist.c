/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

INLINED ExtendedMove *create_promotions(ExtendedMove *movelist, square_t to, direction_t direction)
{
    (movelist++)->move = create_promotion(to - direction, to, QUEEN);
    (movelist++)->move = create_promotion(to - direction, to, ROOK);
    (movelist++)->move = create_promotion(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion(to - direction, to, KNIGHT);

    return movelist;
}

INLINED ExtendedMove *create_underpromotions(
    ExtendedMove *movelist, square_t to, direction_t direction)
{
    (movelist++)->move = create_promotion(to - direction, to, ROOK);
    (movelist++)->move = create_promotion(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion(to - direction, to, KNIGHT);

    return movelist;
}

void place_top_move(ExtendedMove *begin, ExtendedMove *end)
{
    ExtendedMove tmp, *top = begin;

    // Locate the move with the highest score in the specified range.
    for (ExtendedMove *i = begin + 1; i < end; ++i)
        if (i->score > top->score) top = i;

    // Then swap it with the first move of the list.
    tmp = *top;
    *top = *begin;
    *begin = tmp;
}

ExtendedMove *generate_piece_moves(
    ExtendedMove *movelist, const Board *board, color_t us, piecetype_t pt, bitboard_t target)
{
    bitboard_t bb = piece_bb(board, us, pt);
    bitboard_t occupancy = occupancy_bb(board);

    // Iterate through all pieces of the same type and color, and push the moves
    // for which the arrival square mask intersects the targeted bitboard.
    while (bb)
    {
        square_t from = bb_pop_first_sq(&bb);
        bitboard_t b = piece_moves(pt, from, occupancy) & target;

        while (b) (movelist++)->move = create_move(from, bb_pop_first_sq(&b));
    }

    return movelist;
}

ExtendedMove *generate_pawn_capture_moves(
    ExtendedMove *movelist, const Board *board, color_t us, bitboard_t theirPieces, bool inQsearch)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);

    // Start by handling the potential promotions.
    if (pawnsOnLastRank)
    {
        bitboard_t promote = relative_shift_up(pawnsOnLastRank, us);

        // Generate all direct promotions.
        for (bitboard_t b = promote & empty; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush);
        }

        // Generate all promotions with a capture on the left.
        for (bitboard_t b = shift_left(promote) & theirPieces; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush - WEST, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush + WEST);
        }

        // Generate all promotions with a capture on the right.
        for (bitboard_t b = shift_right(promote) & theirPieces; b;)
        {
            square_t to = bb_pop_first_sq(&b);

            (movelist++)->move = create_promotion(to - pawnPush - EAST, to, QUEEN);
            if (!inQsearch) movelist = create_underpromotions(movelist, to, pawnPush + EAST);
        }
    }

    bitboard_t capture = relative_shift_up(pawnsNotOnLastRank, us);

    // Generate all captures on the left.
    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    // Generate all captures on the right.
    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    // Generate en-passant captures.
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return movelist;
}

ExtendedMove *generate_captures(ExtendedMove *movelist, const Board *board, bool inQsearch)
{
    color_t us = board->sideToMove;
    bitboard_t target = color_bb(board, not_color(us));
    square_t kingSquare = get_king_square(board, us);

    // Generate pawn captures/promotions.
    movelist = generate_pawn_capture_moves(movelist, board, us, target, inQsearch);

    // Generate piece captures.
    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    // Generate King captures.
    for (bitboard_t b = king_moves(kingSquare) & target; b;)
        (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    return movelist;
}

ExtendedMove *generate_quiet_pawn_moves(ExtendedMove *movelist, const Board *board, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsNotOnLastRank =
        piece_bb(board, us, PAWN) & ~(us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & empty;

    // Generate simple push moves.
    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    // Generate double push moves.
    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    return movelist;
}

ExtendedMove *generate_quiet(ExtendedMove *movelist, const Board *board)
{
    color_t us = board->sideToMove;
    bitboard_t target = ~occupancy_bb(board);

    // Generate all quiet non-King moves.
    movelist = generate_quiet_pawn_moves(movelist, board, us);
    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    square_t kingSquare = get_king_square(board, us);
    bitboard_t b = king_moves(kingSquare) & target;

    // Generate all King moves.
    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    int kingside = (us == WHITE) ? WHITE_OO : BLACK_OO;
    int queenside = (us == WHITE) ? WHITE_OOO : BLACK_OOO;

    // Generate a kingside castling move if it exists.
    if (!castling_blocked(board, kingside) && (board->stack->castlings & kingside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[kingside]);

    // Generate a queenside castling move if it exists.
    if (!castling_blocked(board, queenside) && (board->stack->castlings & queenside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[queenside]);

    return movelist;
}

ExtendedMove *generate_classic_pawn_moves(ExtendedMove *movelist, const Board *board, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t theirPieces = color_bb(board, not_color(us));
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & empty;

    // Generate simple push moves.
    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    // Generate double push moves.
    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    // Generate all promotions.
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

    // Generate all captures on the left.
    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    // Generate all captures on the right.
    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    // Generate en-passant captures.
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return movelist;
}

ExtendedMove *generate_classic(ExtendedMove *movelist, const Board *board)
{
    color_t us = board->sideToMove;
    bitboard_t target = ~color_bb(board, us);

    // Generate all Pawn moves.
    movelist = generate_classic_pawn_moves(movelist, board, us);

    // Generate all piece moves.
    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    square_t kingSquare = get_king_square(board, us);
    bitboard_t b = king_moves(kingSquare) & target;

    // Generate all King moves.
    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    int kingside = (us == WHITE) ? WHITE_OO : BLACK_OO;
    int queenside = (us == WHITE) ? WHITE_OOO : BLACK_OOO;

    // Generate a kingside castling move if it exists.
    if (!castling_blocked(board, kingside) && (board->stack->castlings & kingside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[kingside]);

    // Generate a queenside castling move if it exists.
    if (!castling_blocked(board, queenside) && (board->stack->castlings & queenside))
        (movelist++)->move = create_castling(kingSquare, board->castlingRookSquare[queenside]);

    return movelist;
}

ExtendedMove *generate_pawn_evasion_moves(
    ExtendedMove *movelist, const Board *board, bitboard_t blockSquares, color_t us)
{
    int pawnPush = pawn_direction(us);
    bitboard_t pawnsOnLastRank = piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t pawnsNotOnLastRank = piece_bb(board, us, PAWN) & ~pawnsOnLastRank;
    bitboard_t empty = ~occupancy_bb(board);
    bitboard_t theirPieces = color_bb(board, not_color(us)) & blockSquares;
    bitboard_t push = relative_shift_up(pawnsNotOnLastRank, us) & empty;
    bitboard_t push2 = relative_shift_up(push & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & empty;

    push &= blockSquares;
    push2 &= blockSquares;

    // Generate simple push moves blocking the check.
    while (push)
    {
        square_t to = bb_pop_first_sq(&push);
        (movelist++)->move = create_move(to - pawnPush, to);
    }

    // Generate double push moves blocking the check.
    while (push2)
    {
        square_t to = bb_pop_first_sq(&push2);
        (movelist++)->move = create_move(to - pawnPush - pawnPush, to);
    }

    // Generate promotions blocking the check.
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

    // Generate left-captures on the checking piece.
    for (bitboard_t b = shift_left(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - WEST, to);
    }

    // Generate right-captures on the checking piece.
    for (bitboard_t b = shift_right(capture) & theirPieces; b;)
    {
        square_t to = bb_pop_first_sq(&b);
        (movelist++)->move = create_move(to - pawnPush - EAST, to);
    }

    // Generate en-passant captures removing the checking Pawn, or blocking the check.
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        if (!(blockSquares & square_bb(board->stack->enPassantSquare - pawnPush))) return movelist;

        bitboard_t captureEnPassant =
            pawnsNotOnLastRank & pawn_moves(board->stack->enPassantSquare, not_color(us));

        while (captureEnPassant)
            (movelist++)->move = create_en_passant(
                bb_pop_first_sq(&captureEnPassant), board->stack->enPassantSquare);
    }

    return movelist;
}

ExtendedMove *generate_evasions(ExtendedMove *movelist, const Board *board)
{
    color_t us = board->sideToMove;
    square_t kingSquare = get_king_square(board, us);
    bitboard_t sliderAttacks = 0;
    bitboard_t sliders = board->stack->checkers & ~piecetypes_bb(board, KNIGHT, PAWN);

    // Compute the bitboard of all checking sliders' attacks.
    while (sliders)
    {
        square_t checkSquare = bb_pop_first_sq(&sliders);
        sliderAttacks |= LineBB[checkSquare][kingSquare] ^ square_bb(checkSquare);
    }

    bitboard_t b = king_moves(kingSquare) & ~color_bb(board, us) & ~sliderAttacks;

    // Generate the list of King moves that evade the initial checking sliders.
    while (b) (movelist++)->move = create_move(kingSquare, bb_pop_first_sq(&b));

    // If in check in multiple times, we know only King moves can be legal.
    if (more_than_one(board->stack->checkers)) return movelist;

    square_t checkSquare = bb_first_sq(board->stack->checkers);
    bitboard_t target = between_bb(checkSquare, kingSquare) | square_bb(checkSquare);

    // Generate the list of Pawn moves blocking the check or capturing the
    // checking piece.
    movelist = generate_pawn_evasion_moves(movelist, board, target, us);

    // Do the same for other pieces.
    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, us, pt, target);

    return movelist;
}

ExtendedMove *generate_all(ExtendedMove *movelist, const Board *board)
{
    color_t us = board->sideToMove;
    bitboard_t pinned = board->stack->kingBlockers[us] & color_bb(board, us);
    square_t kingSquare = get_king_square(board, us);
    ExtendedMove *current = movelist;

    // Use a different move generation if we are in check.
    movelist = board->stack->checkers ? generate_evasions(movelist, board)
                                      : generate_classic(movelist, board);

    // Perform legality verifications while using the fact that only a few moves
    // can be illegal at this point, allowing us to do lighter verifications on
    // legality.
    while (current < movelist)
    {
        if ((pinned || from_sq(current->move) == kingSquare
                || move_type(current->move) == EN_PASSANT)
            && !move_is_legal(board, current->move))
            current->move = (--movelist)->move;
        else
            ++current;
    }

    return movelist;
}
