/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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
#define ENDGAME_H

#include "board.h"

enum
{
    EGTB_SIZE = 2048
};

// Maps a square relative to the given color.
INLINED square_t normalize_square(const Board *board, color_t winning, square_t sq)
{
    if (sq_file(bb_first_sq(piece_bb(board, winning, PAWN))) >= FILE_E) sq ^= FILE_H;

    return (relative_sq(sq, winning));
}

// Bonus for pieces on the edge of the board.
INLINED score_t edge_bonus(square_t sq)
{
    int rank = sq_rank(sq);
    int file = sq_file(sq);

    if (rank > 3) rank ^= 7;
    if (file > 3) file ^= 7;

    return (50 - 2 * (file * file + rank * rank));
}

// Bonus for pieces close to each other.
INLINED score_t close_bonus(square_t sq1, square_t sq2)
{
    return (70 - 10 * SquareDistance[sq1][sq2]);
}

// Bonus for pieces far from each other.
INLINED score_t away_bonus(square_t sq1, square_t sq2)
{
    return (10 + 10 * SquareDistance[sq1][sq2]);
}

// Typedef for specialized endgame scoring functions
typedef score_t (*endgame_score_func_t)(const Board *, color_t);

// Typedef for specialized endgame scaling functions
typedef int (*endgame_scale_func_t)(const Board *, color_t);

// Struct holding a specialized endgame
typedef struct _EndgameEntry
{
    hashkey_t key;
    endgame_score_func_t scoreFunc;
    endgame_scale_func_t scaleFunc;
    color_t winningSide;
} EndgameEntry;

// Global table for hasing endgames
extern EndgameEntry EndgameTable[EGTB_SIZE];

// Initializes the endgame table.
void init_endgame_table(void);

// Initializes the KPK bitbase.
void init_kpk_bitbase(void);

// Checks if the given KPK endgame is winning.
bool kpk_is_winning(color_t stm, square_t bksq, square_t wksq, square_t psq);

// Generic function for all drawn endgames.
score_t eval_draw(const Board *board, color_t winningSide);

// A list of all specialized endgames.
score_t eval_krkn(const Board *board, color_t winningSide);
score_t eval_krkp(const Board *board, color_t winningSide);
score_t eval_krkb(const Board *board, color_t winningSide);
score_t eval_kbnk(const Board *board, color_t winningSide);
score_t eval_kqkr(const Board *board, color_t winningSide);
score_t eval_kqkp(const Board *board, color_t winningSide);
score_t eval_kpk(const Board *board, color_t winningSide);
score_t eval_knnkp(const Board *board, color_t winningSide);

int scale_kpsk(const Board *board, color_t winningSide);
int scale_kbpsk(const Board *board, color_t winningSide);

// Probes the endgame table for the given board.
const EndgameEntry *endgame_probe(const Board *board);

const EndgameEntry *endgame_probe_scalefactor(const Board *board);

#endif
