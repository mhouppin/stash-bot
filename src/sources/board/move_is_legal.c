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

bool    move_is_legal(const board_t *board, move_t move)
{
    color_t     us = board->side_to_move;
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);

    if (move_type(move) == EN_PASSANT)
    {
        // Checks for any discovered check with the en-passant capture.

        square_t    king_square = get_king_square(board, us);
        square_t    capture_square = to - pawn_direction(us);
        bitboard_t  occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
            | square_bb(to);

        return (!(rook_moves_bb(king_square, occupied)
            & pieces_bb(board, not_color(us), QUEEN, ROOK))
            && !(bishop_moves_bb(king_square, occupied)
            & pieces_bb(board, not_color(us), QUEEN, BISHOP)));
    }

    if (move_type(move) == CASTLING)
    {
        // Checks for any opponent piece attack along the king path.

        to = relative_sq((to > from ? SQ_G1 : SQ_C1), us);

        direction_t side = (to > from ? WEST : EAST);

        for (square_t sq = to; sq != from; sq += side)
            if (attackers_to(board, sq) & color_bb(board, not_color(us)))
                return (false);

        return (!board->chess960
            || !(rook_moves_bb(to, occupancy_bb(board) ^ square_bb(to_sq(move)))
                & pieces_bb(board, not_color(us), ROOK, QUEEN)));
    }

    // Checks for any opponent piece attack on the arrival king square.

    if (piece_type(piece_on(board, from)) == KING)
        return (!(attackers_to(board, to) & color_bb(board, not_color(us))));

    // If the moving piece is pinned, checks if the move generates
    // a discovered check.

    return (!(board->stack->king_blockers[us] & square_bb(from))
        || sq_aligned(from, to, get_king_square(board, us)));
}
