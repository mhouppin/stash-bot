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

bool    move_gives_check(const board_t *board, move_t move)
{
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    square_t    capture_square;
    square_t    king_from, rook_from, king_to, rook_to;
    bitboard_t  occupied;

    if (board->stack->check_squares[piece_type(piece_on(board, from))] & square_bb(to))
        return (true);

    square_t    their_king = get_king_square(board, not_color(board->side_to_move));

    if ((board->stack->king_blockers[not_color(board->side_to_move)] & square_bb(from))
        && !sq_aligned(from, to, their_king))
        return (true);

    switch (move_type(move))
    {
        case NORMAL_MOVE:
            return (false);

        case PROMOTION:
            return (piece_moves(promotion_type(move), to, occupancy_bb(board) ^ square_bb(from))
                & square_bb(their_king));

        case EN_PASSANT:
            capture_square = create_sq(sq_file(to), sq_rank(from));

            occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
                | square_bb(to);

            return ((rook_moves_bb(their_king, occupied)
                & pieces_bb(board, board->side_to_move, QUEEN, ROOK))
                | (bishop_moves_bb(their_king, occupied)
                & pieces_bb(board, board->side_to_move, QUEEN, BISHOP)));

        case CASTLING:
            king_from = from;
            rook_from = to;
            king_to = relative_sq(rook_from > king_from ? SQ_G1 : SQ_C1, board->side_to_move);
            rook_to = relative_sq(rook_from > king_from ? SQ_F1 : SQ_D1, board->side_to_move);

            return ((PseudoMoves[ROOK][rook_to] & square_bb(their_king))
                && (rook_moves_bb(rook_to,
                (occupancy_bb(board) ^ square_bb(king_from) ^ square_bb(rook_from))
                | square_bb(king_to) | square_bb(rook_to)) & square_bb(their_king)));

        default:
            __builtin_unreachable();
            return (false);
    }
}
