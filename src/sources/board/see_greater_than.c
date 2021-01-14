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

bool    see_greater_than(const board_t *board, move_t m, score_t threshold)
{
    if (type_of_move(m) != NORMAL_MOVE)
        return (threshold <= 0);

    square_t    from = move_from_square(m);
    square_t    to = move_to_square(m);
    score_t     next_score = PieceScores[MIDGAME][type_of_piece(piece_on(board, to))] - threshold;

    if (next_score < 0)
        return (false);

    next_score = PieceScores[MIDGAME][type_of_piece(piece_on(board, from))] - next_score;

    if (next_score <= 0)
        return (true);

    bitboard_t  occupied = board->piecetype_bits[ALL_PIECES]
        ^ square_bit(from) ^ square_bit(to);
    color_t     side_to_move = color_of_piece(piece_on(board, from));
    bitboard_t  attackers = attackers_list(board, to, occupied);
    bitboard_t  stm_attackers;
    bitboard_t  b;
    int         result = 1;

    while (true)
    {
        side_to_move = not_color(side_to_move);
        attackers &= occupied;

        if (!(stm_attackers = attackers & board->color_bits[side_to_move]))
            break ;

        if (board->stack->pinners[not_color(side_to_move)] & occupied)
        {
            stm_attackers &= ~board->stack->king_blockers[side_to_move];
            if (!stm_attackers)
                break ;
        }

        result ^= 1;

        if ((b = stm_attackers & board->piecetype_bits[PAWN]))
        {
            if ((next_score = PAWN_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bit(first_square(b));
            attackers |= bishop_move_bits(to, occupied)
                & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stm_attackers & board->piecetype_bits[KNIGHT]))
        {
            if ((next_score = KNIGHT_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bit(first_square(b));
        }
        else if ((b = stm_attackers & board->piecetype_bits[BISHOP]))
        {
            if ((next_score = BISHOP_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bit(first_square(b));
            attackers |= bishop_move_bits(to, occupied)
                & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stm_attackers & board->piecetype_bits[ROOK]))
        {
            if ((next_score = ROOK_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bit(first_square(b));
            attackers |= rook_move_bits(to, occupied)
                & piecetypes_bb(board, ROOK, QUEEN);
        }
        else if ((b = stm_attackers & board->piecetype_bits[QUEEN]))
        {
            if ((next_score = QUEEN_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bit(first_square(b));
            attackers |= bishop_move_bits(to, occupied)
                & piecetypes_bb(board, BISHOP, QUEEN);
            attackers |= rook_move_bits(to, occupied)
                & piecetypes_bb(board, ROOK, QUEEN);
        }
        else
            return ((attackers & ~board->color_bits[side_to_move])
                ? result ^ 1 : result);
    }
    return (result);
}
