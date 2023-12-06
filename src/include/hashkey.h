/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#include "types.h"

// Typedef for hashing keys
typedef uint64_t hashkey_t;

// Multiplies two 64-bit unsigned integers and returns the high 64 bits of the result.
INLINED uint64_t mul_hi64(uint64_t x, uint64_t n)
{
    uint64_t xlo = (uint32_t)x;
    uint64_t xhi = x >> 32;
    uint64_t nlo = (uint32_t)n;
    uint64_t nhi = n >> 32;
    uint64_t c1 = (xlo * nlo) >> 32;
    uint64_t c2 = (xhi * nlo) + c1;
    uint64_t c3 = (xlo * nhi) + (uint32_t)c2;

    return xhi * nhi + (c2 >> 32) + (c3 >> 32);
}

// Global table for Zobrist Piece-Square hashes
extern hashkey_t ZobristPsq[PIECE_NB][SQUARE_NB];

// Global table for Zobrist Enpassant hashes
extern hashkey_t ZobristEnPassant[FILE_NB];

// Global table for Zobrist Castling hashes
extern hashkey_t ZobristCastling[CASTLING_NB];

// Global value for Zobrist STM hash
extern hashkey_t ZobristSideToMove;

void zobrist_init(void);

#endif // HASHKEY_H
