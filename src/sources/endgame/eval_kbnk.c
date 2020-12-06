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

#include <stdlib.h>
#include "endgame.h"

score_t eval_kbnk(const board_t *board, color_t winning)
{
    score_t     base_score = VICTORY + BISHOP_EG_SCORE + KNIGHT_EG_SCORE;
    square_t    losing_ksq = board_king_square(board, not_color(winning));
    square_t    winning_ksq = board_king_square(board, winning);

    if (piecetype_bb(board, BISHOP) & DARK_SQUARES)
        losing_ksq ^= SQ_A8;

    file_t      file = file_of_square(losing_ksq);
    rank_t      rank = rank_of_square(losing_ksq);

    base_score += abs(file * file - rank * rank) * 10;
    base_score -= SquareDistance[losing_ksq][winning_ksq] * 16;

    return (15 + (board->side_to_move == winning ? base_score : -base_score));
}
