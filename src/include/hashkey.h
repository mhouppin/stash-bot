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

#ifndef HASHKEY_H
#define HASHKEY_H

#include "chess_types.h"
#include "core.h"

// Typedef for hashing keys
typedef u64 Key;

// Multiplies two u64s and returns the high 64 bits of the result
INLINED u64 u64_mulhi(u64 lhs, u64 rhs) {
#ifdef HAS_INT128
    return ((u128)lhs * (u128)rhs) >> 64;
#else
    u64 llo = (u32)lhs;
    u64 lhi = lhs >> 32;
    u64 rlo = (u32)rhs;
    u64 rhi = rhs >> 32;
    u64 c1 = (llo * rlo) >> 32;
    u64 c2 = (lhi * rlo) + c1;
    u64 c3 = (llo * rhi) + (u32)c2;

    return lhi * rhi + (c2 >> 32) + (c3 >> 32);
#endif
}

// Global table for Zobrist Piece-Square hashes
extern Key ZobristPsq[PIECE_NB][SQUARE_NB];

// Global table for Zobrist en passant hashes
extern Key ZobristEnPassant[FILE_NB];

// Global table for Zobrist castling hashes
extern Key ZobristCastling[CASTLING_NB];

// Global value for Zobrist STM hash
extern Key ZobristSideToMove;

void zobrist_init(void);

#endif
