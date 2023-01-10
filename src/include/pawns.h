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

#ifndef PAWNS_H
#define PAWNS_H

#include "board.h"

// Struct for pawn eval data
typedef struct _PawnEntry
{
    hashkey_t key;
    bitboard_t attackSpan[COLOR_NB];
    bitboard_t attacks[COLOR_NB];
    bitboard_t attacks2[COLOR_NB];
    bitboard_t passed[COLOR_NB];
    scorepair_t value;
} PawnEntry;

enum
{
    PawnTableSize = 1 << 15
};

// Probes the pawn hash table for the given position.
PawnEntry *pawn_probe(const Board *board);

#endif
