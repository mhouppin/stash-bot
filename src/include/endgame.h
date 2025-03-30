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

enum {
    ENDGAME_TABLE_SIZE = 2048
};

INLINED Square normalize_square(Color strong_side, Square square, bool flip_file) {
    return square_relative(square, strong_side) ^ (flip_file * 0b111u);
}

// Bonus for pieces close to the corner.
INLINED Score corner_bonus(Square square) {
    const Rank rank = square_rank(square);
    const File file = square_file(square);
    const i16 rdist = i16_min(rank, rank ^ 0b111u);
    const i16 fdist = i16_min(file, file ^ 0b111u);

    return 50 - 2 * (fdist * fdist + rdist * rdist);
}

// Bonus for pieces close to each other.
INLINED Score close_bonus(Square square1, Square square2) {
    return 70 - 10 * square_distance(square1, square2);
}

// Bonus for pieces far from each other.
INLINED Score away_bonus(Square square1, Square square2) {
    return 10 + 10 * square_distance(square1, square2);
}

// Typedef for specialized endgame scoring functions
typedef Score (*EndgameScoreFn)(const Board *, Color);

// Typedef for specialized endgame scaling functions
typedef Scalefactor (*EndgameScaleFn)(const Board *, Color);

// Struct holding a specialized endgame
typedef struct {
    Key key;
    EndgameScoreFn score_fn;
    EndgameScaleFn scale_fn;
    Color strong_side;
} EndgameEntry;

// Initializes the endgame table
void endgame_table_init(void);

// Generic function for all drawn endgames
Score eval_draw(const Board *board, Color strong_side);

// Specialized scoring endgames
Score eval_kpk(const Board *board, Color strong_side);
Score eval_kbnk(const Board *board, Color strong_side);
Score eval_krkp(const Board *board, Color strong_side);
Score eval_krkn(const Board *board, Color strong_side);
Score eval_krkb(const Board *board, Color strong_side);
Score eval_kqkp(const Board *board, Color strong_side);
Score eval_kqkr(const Board *board, Color strong_side);
Score eval_knnkp(const Board *board, Color strong_side);
Score eval_kmpkn(const Board *board, Color strong_side);
Score eval_kmpkb(const Board *board, Color strong_side);
Score eval_krpkr(const Board *board, Color strong_side);

Scalefactor scale_kpsk(const Board *board, Color strong_side);
Scalefactor scale_kbpsk(const Board *board, Color strong_side);
Scalefactor scale_kqkrps(const Board *board, Color strong_side);

// Probes the endgame table for the given board. Returns a pointer to the corresponding endgame if
// found, NULL otherwise.
const EndgameEntry *endgame_probe_score(const Board *board);

// Same as above, except that this one probes for a scaling function rather than a scoring one.
const EndgameEntry *endgame_probe_scale(const Board *board);

#endif
