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

    square_t    king_square = get_king_square(board, WHITE);
    bitboard_t  b = king_moves(king_square) & target;

    while (b)
        (movelist++)->move = create_move(king_square, bb_pop_first_sq(&b));

    if (!castling_blocked(board, WHITE_OO) && (board->stack->castlings & WHITE_OO))
        (movelist++)->move = create_castling(king_square, board->castling_rook_square[WHITE_OO]);

    if (!castling_blocked(board, WHITE_OOO) && (board->stack->castlings & WHITE_OOO))
        (movelist++)->move = create_castling(king_square, board->castling_rook_square[WHITE_OOO]);

    return (movelist);
}

extmove_t   *generate_black_quiet(extmove_t *movelist, const board_t *board, bitboard_t target)
{
    movelist = generate_quiet_pawn_moves(movelist, board, BLACK);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        movelist = generate_piece_moves(movelist, board, BLACK, pt, target);

    square_t    king_square = get_king_square(board, BLACK);
    bitboard_t  b = king_moves(king_square) & target;

    while (b)
        (movelist++)->move = create_move(king_square, bb_pop_first_sq(&b));

    if (!castling_blocked(board, BLACK_OO) && (board->stack->castlings & BLACK_OO))
        (movelist++)->move = create_castling(king_square, board->castling_rook_square[BLACK_OO]);

    if (!castling_blocked(board, BLACK_OOO) && (board->stack->castlings & BLACK_OOO))
        (movelist++)->move = create_castling(king_square, board->castling_rook_square[BLACK_OOO]);

    return (movelist);
}

extmove_t   *generate_quiet(extmove_t *movelist, const board_t *board)
{
    color_t     us = board->side_to_move;
    bitboard_t  target = ~occupancy_bb(board);

    return (us == WHITE ? generate_white_quiet(movelist, board, target)
        : generate_black_quiet(movelist, board, target));
}
