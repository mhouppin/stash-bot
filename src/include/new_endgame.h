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

#include "new_board.h"

enum {
    ENDGAME_TABLE_SIZE = 2048
};

// Bonus for pieces close to the corner.
INLINED Score corner_bonus(Square square) {
    const Rank rank = square_rank(square);
    const File file = square_file(square);
    const i16 rdist = i16_min(rank, rank ^ 7);
    const i16 fdist = i16_min(file, file ^ 7);

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

#endif
