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

#ifndef TT_H
#define TT_H

#include "chess_types.h"
#include "hashkey.h"

enum {
    ENTRY_CLUSTER_SIZE = 4,

    GENERATION_SHIFT = 4,
    GENERATION_MASK = 256 - GENERATION_SHIFT,
    GENERATION_CYCLE = 256 + GENERATION_SHIFT - 1,
};

typedef struct {
    Key key;
    Score score;
    Score eval;
    u8 depth;
    u8 genbound;
    Move bestmove;
} TranspositionEntry;

INLINED i16 tt_entry_replace_score(const TranspositionEntry *tt_entry, u8 generation) {
    return (i16)tt_entry->depth
        - (((i16)GENERATION_CYCLE + (i16)generation - (i16)tt_entry->genbound) & GENERATION_MASK);
}

INLINED Bound tt_entry_bound(const TranspositionEntry *tt_entry) {
    return (Bound)(tt_entry->genbound & ~GENERATION_MASK);
}

typedef struct {
    TranspositionEntry cluster_entry[ENTRY_CLUSTER_SIZE];
} TranspositionCluster;

// Required for correct prefetching and structure alignment
static_assert(
    64 % sizeof(TranspositionCluster) == 0,
    "Clusters are not aligned to cache boundaries"
);

typedef struct {
    usize cluster_count;
    TranspositionCluster *table;
    u8 generation;
} TranspositionTable;

// Returns the entry cluster for the given hashkey
INLINED TranspositionEntry *tt_entry_at(TranspositionTable *tt, Key key) {
    return tt->table[u64_mulhi(key, tt->cluster_count)].cluster_entry;
}

INLINED void tt_new_search(TranspositionTable *tt) {
    tt->generation += GENERATION_SHIFT;
}

INLINED Score score_to_tt(Score score, u16 plies_from_root) {
    return score >= MATE_FOUND ? score + plies_from_root
        : score <= -MATE_FOUND ? score - plies_from_root
                               : score;
}

INLINED Score score_from_tt(Score score, u16 plies_from_root) {
    return score >= MATE_FOUND ? score - plies_from_root
        : score <= -MATE_FOUND ? score + plies_from_root
                               : score;
}

void tt_init(TranspositionTable *tt);

void tt_destroy(TranspositionTable *tt);

// Clears the TT contents before starting a new game
void tt_init_new_game(TranspositionTable *tt, usize thread_count);

// Returns data matching the given key
TranspositionEntry *tt_probe(TranspositionTable *tt, Key key, bool *found);

// Saves the given entry in the TT
void tt_save(
    TranspositionTable *tt,
    TranspositionEntry *tt_entry,
    Key key,
    Score score,
    Score eval,
    i16 depth,
    Bound bound,
    Move bestmove
);

// Returns the filled proportion of the TT (per mil).
u16 tt_hashfull(TranspositionTable *tt);

void tt_resize(TranspositionTable *tt, usize size_mb, usize thread_count);

#endif
