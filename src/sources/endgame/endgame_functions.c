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

#include "endgame.h"

INLINED score_t     edge_bonus(square_t sq)
{
    int rank = sq_rank(sq);
    int file = sq_file(sq);

    if (rank > 3) rank ^= 7;
    if (file > 3) file ^= 7;

    return (50 - 2 * (file * file + rank * rank));
}

INLINED score_t     close_bonus(square_t sq1, square_t sq2)
{
    return (70 - 10 * SquareDistance[sq1][sq2]);
}

INLINED score_t     away_bonus(square_t sq1, square_t sq2)
{
    return (10 + 10 * SquareDistance[sq1][sq2]);
}

score_t eval_draw(const board_t *board, color_t winning_side)
{
    (void)board;
    (void)winning_side;
    return (0);
}

score_t eval_krkp(const board_t *board, color_t winning_side)
{
    square_t    winning_ksq = get_king_square(board, winning_side);
    square_t    losing_ksq = get_king_square(board, not_color(winning_side));
    square_t    winning_rook = bb_first_sq(piecetype_bb(board, ROOK));
    square_t    losing_pawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t    push_square = losing_pawn + pawn_direction(not_color(winning_side));
    square_t    promote_square = create_sq(sq_file(losing_pawn), relative_rank(RANK_8, not_color(winning_side)));
    score_t     score;

    // Does the winning King control the promotion path ?

    if (forward_file_bb(winning_side, winning_ksq) & square_bb(losing_pawn))
        score = ROOK_EG_SCORE - SquareDistance[winning_ksq][losing_pawn];

    // Is the losing King unable to reach either the Rook or its Pawn ?

    else if (SquareDistance[losing_ksq][losing_pawn] >= 3 + (board->side_to_move != winning_side)
        && SquareDistance[losing_ksq][winning_rook] >= 3)
        score = ROOK_EG_SCORE - SquareDistance[winning_ksq][losing_pawn];

    // Is the Pawn close to the promotion square and defended by its King ?
    // Also, is the winning King unable to reach the Pawn ?

    else if (relative_rank(winning_side, losing_ksq) <= RANK_3
        && SquareDistance[losing_ksq][losing_pawn] == 1
        && relative_rank(winning_side, winning_ksq) >= RANK_4
        && SquareDistance[winning_ksq][losing_pawn] >= 3 + (board->side_to_move == winning_side))
        score = 40 - 4 * SquareDistance[winning_ksq][losing_pawn];

    else
        score = 100 - 4 * (SquareDistance[winning_ksq][push_square]
            - SquareDistance[losing_ksq][push_square]
            - SquareDistance[losing_pawn][promote_square]);

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_krkn(const board_t *board, color_t winning_side)
{
    square_t    losing_ksq = get_king_square(board, not_color(winning_side));
    square_t    losing_knight = bb_first_sq(piecetype_bb(board, KNIGHT));
    score_t     score = edge_bonus(losing_ksq) + away_bonus(losing_ksq, losing_knight);

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_krkb(const board_t *board, color_t winning_side)
{
    score_t score = edge_bonus(get_king_square(board, not_color(winning_side)));

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_kbnk(const board_t *board, color_t winning_side)
{
    square_t    losing_ksq = get_king_square(board, not_color(winning_side));
    square_t    winning_ksq = get_king_square(board, winning_side);
    score_t     score = VICTORY + KNIGHT_MG_SCORE + BISHOP_MG_SCORE;

    score += 70 - 10 * SquareDistance[losing_ksq][winning_ksq];

    // Don't push the king to the wrong corner

    if (piecetype_bb(board, BISHOP) && DARK_SQUARES)
        losing_ksq ^= SQ_A8;

    score += abs(sq_file(losing_ksq) - sq_rank(losing_ksq)) * 100;

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_kqkr(const board_t *board, color_t winning_side)
{
    score_t     score = QUEEN_EG_SCORE - ROOK_EG_SCORE;
    square_t    losing_ksq = get_king_square(board, not_color(winning_side));
    square_t    winning_ksq = get_king_square(board, winning_side);

    score += edge_bonus(losing_ksq);
    score += close_bonus(winning_ksq, losing_ksq);

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_kqkp(const board_t *board, color_t winning_side)
{
    square_t    losing_ksq = get_king_square(board, not_color(winning_side));
    square_t    losing_pawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t    winning_ksq = get_king_square(board, winning_side);
    score_t     score = close_bonus(winning_ksq, losing_ksq);

    if (relative_sq_rank(losing_pawn, not_color(winning_side)) != RANK_7
        || SquareDistance[losing_ksq][losing_pawn] != 1
        || ((FILE_B_BITS | FILE_D_BITS | FILE_E_BITS | FILE_G_BITS) & square_bb(losing_pawn)))
        score += QUEEN_EG_SCORE - PAWN_EG_SCORE;

    return (board->side_to_move == winning_side ? score : -score);
}

score_t eval_knnkp(const board_t *board, color_t winning_side)
{
    score_t score = PAWN_EG_SCORE;

    score += edge_bonus(get_king_square(board, not_color(winning_side)));
    score -= 5 * relative_sq_rank(bb_first_sq(piecetype_bb(board, PAWN)), not_color(winning_side));

    return (board->side_to_move == winning_side ? score : -score);
}