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

#ifndef TT_H
#define TT_H

#include <stdatomic.h>

#include "new_chess_types.h"
#include "new_hashkey.h"

enum {
    ENTRY_CLUSTER_SIZE = 4,

    GENERATION_SHIFT = 4,
    GENERATION_MASK = 256 - GENERATION_SHIFT,
    GENERATION_CYCLE = 256 + GENERATION_SHIFT - 1,
};

typedef struct _OpaqueEntry {
    _Atomic Key key;
    _Atomic u64 entry_data;
} OpaqueEntry;

typedef struct _TranspositionCluster {
    OpaqueEntry cluster_entry[ENTRY_CLUSTER_SIZE];
} TranspositionCluster;

// Required for correct prefetching and structure alignment
static_assert(64 % sizeof(TranspositionCluster) == 0);

typedef struct _TranspositionEntry {
    Key key;
    Bound bound;
    i16 depth;
    Score eval;
    Score score;
    Move bestmove;
    OpaqueEntry *entry_ptr;
} TranspositionEntry;

typedef struct _TranspositionTable {
    usize cluster_count;
    TranspositionCluster *table;
    u8 generation;
} TranspositionTable;

// Returns the entry cluster for the given hashkey
INLINED OpaqueEntry *tt_entry_at(TranspositionTable *tt, Key key) {
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

INLINED void tt_entry_set(
    TranspositionEntry *restrict tt_entry,
    OpaqueEntry *restrict opaque_entry,
    Key key,
    u64 entry_data
) {
    tt_entry->key = key;
    tt_entry->bound = (u8)(entry_data & ~GENERATION_MASK);
    tt_entry->depth = (i16)(u8)(entry_data >> 8);
    tt_entry->eval = (Score)(u16)(entry_data >> 16);
    tt_entry->score = (Score)(u16)(entry_data >> 32);
    tt_entry->bestmove = (Move)(entry_data >> 48);
    tt_entry->entry_ptr = opaque_entry;
}

INLINED void write_opaque_entry(
    OpaqueEntry *restrict opaque_entry,
    Key key,
    u8 generation_bound,
    u8 depth,
    Score eval,
    Score score,
    Move bestmove
) {
    u64 entry_data = (u64)generation_bound | ((u64)depth << 8) | ((u64)(u16)eval << 16)
        | ((u64)(u16)score << 32) | ((u64)bestmove << 48);

    atomic_store_explicit(&opaque_entry->key, key, memory_order_relaxed);
    atomic_store_explicit(&opaque_entry->entry_data, entry_data, memory_order_relaxed);
}

void tt_init(TranspositionTable *tt);

void tt_destroy(TranspositionTable *tt);

// Clears the TT contents before starting a new game
void tt_init_new_game(TranspositionTable *tt, usize thread_count);

// Stores data matching the entry for the given key
void tt_probe(TranspositionTable *tt, Key key, TranspositionEntry *tt_entry, bool *found);

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
