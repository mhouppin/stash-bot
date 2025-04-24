/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#ifndef KPK_BITBASE_H
#define KPK_BITBASE_H

#include "chess_types.h"

enum {
    // - 2 possible stm values (WHITE, BLACK);
    // - 24 possible pawn squares (all on the queenside);
    // - 64 possible king squares for each.
    KPK_SIZE = 2 * 24 * 64 * 64,

    // Possible bitmask values during retrograde analysis.
    KPK_INVALID = 0,
    KPK_UNKNOWN = 1,
    KPK_DRAW = 2,
    KPK_WIN = 4
};

typedef struct {
    Square ksq[COLOR_NB];
    Square psq;
    Color stm;
    u8 result;
    // Make the struct size a power of two
    u16 padding;
} KpkPosition;

// Initialize the KPK bitbase
void kpk_bitbase_init(void);

// Check if the given KPK endgame is winning. This function assumes that all
// parameters have been normalized as such:
// - stm is WHITE if the strong side has the move, BLACK otherwise;
// - weak_ksq is the position of the weak King relative to the strong POV;
// - strong_ksq is the position of the strong King relative to the strong POV;
// - psq is the position of the Pawn relative to the strong POV;
// - If the Pawn is on the kingside, all squares passed must have their files flipped.
bool kpk_bitbase_is_winning(Square weak_ksq, Square strong_ksq, Square psq, Color stm);

#endif
