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

#include <stdatomic.h>

#include "chess_types.h"
#include "hashkey.h"

enum {
    ENTRY_CLUSTER_SIZE = 5,

    GENERATION_SHIFT = 4,
    GENERATION_MASK = 256 - GENERATION_SHIFT,
    GENERATION_CYCLE = 256 + GENERATION_SHIFT - 1,
};

typedef struct {
    _Atomic u32 key32;
    _Atomic Score score;
    _Atomic Score eval;
    _Atomic u8 depth;
    _Atomic u8 genbound;
    _Atomic Move bestmove;
} TranspositionEntry;

INLINED u32 tt_entry_key32(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->key32, memory_order_relaxed);
}

INLINED Score tt_entry_score(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->score, memory_order_relaxed);
}

INLINED Score tt_entry_eval(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->eval, memory_order_relaxed);
}

INLINED u8 tt_entry_depth(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->depth, memory_order_relaxed);
}

INLINED u8 tt_entry_genbound(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->genbound, memory_order_relaxed);
}

INLINED Move tt_entry_bestmove(const TranspositionEntry *tt_entry) {
    return atomic_load_explicit(&tt_entry->bestmove, memory_order_relaxed);
}

INLINED i16 tt_entry_replace_score(const TranspositionEntry *tt_entry, u8 generation) {
    return (i16)tt_entry_depth(tt_entry)
        - (((i16)GENERATION_CYCLE + (i16)generation - (i16)tt_entry_genbound(tt_entry))
           & GENERATION_MASK);
}

INLINED Bound tt_entry_bound(const TranspositionEntry *tt_entry) {
    return (Bound)(tt_entry_genbound(tt_entry) & ~GENERATION_MASK);
}

typedef struct {
    TranspositionEntry cluster_entry[ENTRY_CLUSTER_SIZE];
    u8 padding[4];
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
