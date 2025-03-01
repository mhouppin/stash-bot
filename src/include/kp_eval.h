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

#ifndef KP_EVAL_H
#define KP_EVAL_H

#include "board.h"

enum {
    KING_PAWN_ENTRY_NB = 32768,
};

// Struct for pawn eval data
typedef struct {
    Key key;
    Bitboard attack_span[COLOR_NB];
    Bitboard passed[COLOR_NB];
    Scorepair value;
} KingPawnEntry;

typedef struct {
    KingPawnEntry entry[KING_PAWN_ENTRY_NB];
} KingPawnTable;

// Struct for local pawn eval data
typedef struct {
    Bitboard attacks[COLOR_NB];
    Bitboard attacks2[COLOR_NB];
} PawnLocalData;

static_assert(sizeof(KingPawnTable) % 64 == 0, "Misaligned King-Pawn table");

// Probes the King-Pawn hash table for the given position.
KingPawnEntry *king_pawn_probe(const Board *board);

#endif
