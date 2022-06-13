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

#ifndef TT_H
#define TT_H

#include "hashkey.h"
#include "types.h"
#include <string.h>

// Struct for TT entry
typedef struct tt_entry_s
{
    hashkey_t key;
    score_t score;
    score_t eval;
    uint8_t depth;
    uint8_t genbound;
    uint16_t bestmove;
} tt_entry_t;

enum
{
    ClusterSize = 4
};

// Struct for TT entry cluster
typedef struct cluster_s
{
    tt_entry_t clEntry[ClusterSize];
} cluster_t;

// Struct for the transposition table
typedef struct transposition_s
{
    size_t clusterCount;
    cluster_t *table;
    uint8_t generation;
} transposition_t;

// Global transposition table
extern transposition_t TT;

// Returns the entry cluster for the given hashkey.
INLINED tt_entry_t *tt_entry_at(hashkey_t k)
{
    return (TT.table[mul_hi64(k, TT.clusterCount)].clEntry);
}

// Updates the TT generation.
INLINED void tt_clear(void) { TT.generation += 4; }

// Converts a score to a TT score.
INLINED score_t score_to_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s + plies : s <= -MATE_FOUND ? s - plies : s);
}

// Converts a TT score to a score.
INLINED score_t score_from_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s - plies : s <= -MATE_FOUND ? s + plies : s);
}

// Resets the TT contents.
void tt_bzero(size_t threadCount);

// Probes the TT for the given hashkey.
tt_entry_t *tt_probe(hashkey_t key, bool *found);

// Saves the given entry in the TT.
void tt_save(tt_entry_t *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m);

// Returns the filling rate of the TT (per mil).
int tt_hashfull(void);

// Resizes the TT.
void tt_resize(size_t mbsize);

#endif // TT_H
