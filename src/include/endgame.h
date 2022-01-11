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

#ifndef ENDGAME_H
# define ENDGAME_H

# include "board.h"

enum { EGTB_SIZE = 2048 };

INLINED square_t normalize_square(const board_t *board, color_t winning, square_t sq)
{
    if (sq_file(bb_first_sq(piece_bb(board, winning, PAWN))) >= FILE_E)
        sq ^= FILE_H;

    return (relative_sq(sq, winning));
}

INLINED score_t edge_bonus(square_t sq)
{
    int rank = sq_rank(sq);
    int file = sq_file(sq);

    if (rank > 3) rank ^= 7;
    if (file > 3) file ^= 7;

    return (50 - 2 * (file * file + rank * rank));
}

INLINED score_t close_bonus(square_t sq1, square_t sq2)
{
    return (70 - 10 * SquareDistance[sq1][sq2]);
}

INLINED score_t away_bonus(square_t sq1, square_t sq2)
{
    return (10 + 10 * SquareDistance[sq1][sq2]);
}

typedef score_t (*endgame_func_t)(const board_t *, color_t);

typedef struct endgame_entry_s
{
    hashkey_t key;
    endgame_func_t func;
    color_t winningSide;
}
endgame_entry_t;

extern endgame_entry_t EndgameTable[EGTB_SIZE];

void init_endgame_table(void);
void init_kpk_bitbase(void);
bool kpk_is_winning(color_t stm, square_t bksq, square_t wksq, square_t psq);

score_t eval_draw(const board_t *board, color_t winningSide);
score_t eval_krkn(const board_t *board, color_t winningSide);
score_t eval_krkp(const board_t *board, color_t winningSide);
score_t eval_krkb(const board_t *board, color_t winningSide);
score_t eval_kbnk(const board_t *board, color_t winningSide);
score_t eval_kqkr(const board_t *board, color_t winningSide);
score_t eval_kqkp(const board_t *board, color_t winningSide);
score_t eval_kpk(const board_t *board, color_t winningSide);
score_t eval_kpsk(const board_t *board, color_t winningSide);
score_t eval_knnkp(const board_t *board, color_t winningSide);
score_t eval_kbpsk(const board_t *board, color_t winningSide);

const endgame_entry_t *endgame_probe(const board_t *board);

#endif
