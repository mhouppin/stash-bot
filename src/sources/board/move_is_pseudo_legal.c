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
#include "movelist.h"

bool    move_is_pseudo_legal(const board_t *board, move_t move)
{
    color_t     us = board->side_to_move;
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    piece_t     piece = piece_on(board, from);

    // Slower check for uncommon cases.

    if (move_type(move) != NORMAL_MOVE)
    {
        movelist_t  list;

        list_pseudo(&list, board);
        return (movelist_has_move(&list, move));
    }

    // The move is normal, so promotion type bits cannot be set.
    // (Note that this check will likely never trigger, since all recent CPUs
    // guarantee atomic reads/writes to memory.)

    if (promotion_type(move) != KNIGHT)
        return (false);

    // Check if there is a piece that belongs to us on the 'from' square.

    if (piece == NO_PIECE || piece_color(piece) != us)
        return (false);

    // Check if there is the 'to' square doesn't contain a friendly piece.
    // (This works because even though castling is encoded as 'King takes Rook',
    // it is handled in the 'uncommon case' scope.

    if (color_bb(board, us) & square_bb(to))
        return (false);

    if (piece_type(piece) == PAWN)
    {
        // The pawn cannot arrive on a promotion square, since we alerady handled
        // the promotion case.

        if ((RANK_8_BITS | RANK_1_BITS) & square_bb(to))
            return (false);

        // Check if the pawn move is a valid capture, push, or double push.

        if (!(pawn_moves(from, us) & color_bb(board, not_color(us)) & square_bb(to))
            && !((from + pawn_direction(us) == to) && empty_square(board, to))
            && !((from + 2 * pawn_direction(us) == to) && relative_sq_rank(from, us) == RANK_2
                && empty_square(board, to) && empty_square(board, to - pawn_direction(us))))
            return (false);
    }

    // Check if the piece can reach the 'to' square from its position.

    else if (!(piece_moves(piece_type(piece), from, occupancy_bb(board)) & square_bb(to)))
        return (false);

    if (board->stack->checkers)
    {
        if (piece_type(piece) != KING)
        {
            // If double check, only a king move can be legal.

            if (more_than_one(board->stack->checkers))
                return (false);

            // We must either capture the checking piece, or block the attack
            // if it's a slider.

            if (!((between_bb(bb_first_sq(board->stack->checkers), get_king_square(board, us))
                | board->stack->checkers) & square_bb(to)))
                return (false);
        }

        // Check if the King is still under the fire of the opponent's pieces after moving.

        else if (attackers_list(board, to, occupancy_bb(board) ^ square_bb(from))
            & color_bb(board, not_color(us)))
            return (false);
    }

    return (true);
}
