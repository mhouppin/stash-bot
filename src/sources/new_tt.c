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

#include "new_tt.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "new_wmalloc.h"

typedef struct _ResetThreadData {
    TranspositionTable *tt;
    usize cluster_begin;
    usize cluster_end;
    pthread_t this_thread;
} ResetThreadData;

void tt_init(TranspositionTable *tt) {
    tt->cluster_count = 0;
    tt->table = NULL;
    tt->generation = 0;
}

void tt_destroy(TranspositionTable *tt) {
    wrap_aligned_free(tt->table);
}

void *tt_reset_thread_entry_point(void *data) {
    ResetThreadData *reset_thread_data = (ResetThreadData *)data;

    for (usize i = reset_thread_data->cluster_begin; i < reset_thread_data->cluster_end; ++i) {
        for (usize j = 0; j < ENTRY_CLUSTER_SIZE; ++j) {
            write_opaque_entry(
                &reset_thread_data->tt->table[i].cluster_entry[j],
                0,
                0,
                0,
                NO_SCORE,
                NO_SCORE,
                NO_MOVE
            );
        }
    }

    return NULL;
}

void tt_init_new_game(TranspositionTable *tt, usize thread_count) {
    assert(thread_count != 0);
    ResetThreadData *thread_list = wrap_malloc(sizeof(ResetThreadData) * thread_count);

    tt->generation = 0;

    // Define each thread's work range.
    for (usize i = 0; i < thread_count; ++i) {
        thread_list[i].tt = tt;
        thread_list[i].cluster_begin = tt->cluster_count * i / thread_count;
        thread_list[i].cluster_end = tt->cluster_count * (i + 1) / thread_count;
    }

    // Create all helper threads.
    for (usize i = 1; i < thread_count; ++i) {
        if (pthread_create(
                &thread_list[i].this_thread,
                NULL,
                tt_reset_thread_entry_point,
                &thread_list[i]
            )) {
            perror("Unable to create TT reset threads");
            exit(EXIT_FAILURE);
        }
    }

    // Recycle the main thread for zeroing.
    tt_reset_thread_entry_point(&thread_list[0]);

    for (usize i = 1; i < thread_count; ++i) {
        pthread_join(thread_list[i].this_thread, NULL);
    }

    free(thread_list);
}

static i16 entry_score(u64 entry_data, u8 generation) {
    const u8 generation_bound = (u8)(entry_data >> 0);
    const u8 depth = (u8)(entry_data >> 8);

    return (i16)depth
        - (((i16)GENERATION_CYCLE + (i16)generation - (i16)generation_bound) & GENERATION_MASK);
}

void tt_probe(TranspositionTable *tt, Key key, TranspositionEntry *tt_entry, bool *found) {
    OpaqueEntry *cluster = tt_entry_at(tt, key);

    // Try to find an entry matching the given key.
    for (usize i = 0; i < ENTRY_CLUSTER_SIZE; ++i) {
        Key cur_key = atomic_load_explicit(&cluster[i].key, memory_order_relaxed);

        if (cur_key == 0 || cur_key == key) {
            u64 entry_data = atomic_load_explicit(&cluster[i].entry_data, memory_order_relaxed);

            // Refresh the generation counter to prevent it from being cleared.
            entry_data = (entry_data & ~(u64)GENERATION_MASK) | tt->generation;
            atomic_store_explicit(&cluster[i].entry_data, entry_data, memory_order_relaxed);
            *found = (cur_key == key);
            tt_entry_set(tt_entry, &cluster[i], cur_key, entry_data);
            return;
        }
    }

    OpaqueEntry *replace = cluster;

    u64 replace_data = atomic_load_explicit(&cluster->entry_data, memory_order_relaxed);

    // Find the slot with the minimal (depth + generation * 4) score to be replaced.
    for (usize i = 1; i < ENTRY_CLUSTER_SIZE; ++i) {
        u64 entry_data = atomic_load_explicit(&cluster[i].entry_data, memory_order_relaxed);

        if (entry_score(replace_data, tt->generation) > entry_score(entry_data, tt->generation)) {
            replace = &cluster[i];
            replace_data = entry_data;
        }
    }

    *found = false;
    tt_entry_set(
        tt_entry,
        replace,
        atomic_load_explicit(&replace->key, memory_order_relaxed),
        replace_data
    );
}

void tt_save(
    TranspositionTable *tt,
    TranspositionEntry *tt_entry,
    Key key,
    Score score,
    Score eval,
    i16 depth,
    Bound bound,
    Move bestmove
) {
    const Key cur_key = atomic_load_explicit(&tt_entry->entry_ptr->key, memory_order_relaxed);
    u64 cur_data = atomic_load_explicit(&tt_entry->entry_ptr->entry_data, memory_order_relaxed);

    if (cur_key == key) {
        // Keep the old bestmove if we do not have a new one.
        if (bestmove == NO_MOVE) {
            bestmove = (Move)(cur_data >> 48);
        } else {
            cur_data = (cur_data & U64(0x0000FFFFFFFFFFFF)) | ((u64)bestmove << 48);
            atomic_store_explicit(&tt_entry->entry_ptr->entry_data, cur_data, memory_order_relaxed);
        }
    }

    // Do not erase entries with high depth for the same position.
    if (bound == EXACT_BOUND || cur_key != key || depth + 4 >= (i16)(u8)(cur_data >> 8)) {
        write_opaque_entry(
            tt_entry->entry_ptr,
            key,
            tt->generation | bound,
            depth,
            eval,
            score,
            bestmove
        );
    }
}

u16 tt_hashfull(TranspositionTable *tt) {
    u16 count = 0;

    for (usize i = 0; i < 1000; ++i) {
        for (usize j = 0; j < ENTRY_CLUSTER_SIZE; ++j) {
            count += (atomic_load_explicit(
                          &tt->table[i].cluster_entry[j].entry_data,
                          memory_order_relaxed
                      )
                      & GENERATION_MASK)
                == tt->generation;
        }
    }

    return count / ENTRY_CLUSTER_SIZE;
}

void tt_resize(TranspositionTable *tt, usize size_mb, usize thread_count) {
    if (tt->table != NULL) {
        wrap_aligned_free(tt->table);
    }

    // Note: 1000000 is already a multiple of 64, and we static_assert that TranspositionCluster's
    // size divides 64, so we don't need any extra rounding for aligned_alloc() here.
    tt->cluster_count = size_mb * 1024 * 1024 / sizeof(TranspositionCluster);
    tt->table = wrap_aligned_alloc(64, tt->cluster_count * sizeof(TranspositionCluster));
    tt_init_new_game(tt, thread_count);
}
