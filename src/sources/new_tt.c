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
    const TranspositionEntry empty_entry = {
        .key = 0,
        .score = NO_SCORE,
        .eval = NO_SCORE,
        .depth = 0,
        .genbound = 0 | NO_BOUND,
        .bestmove = NO_MOVE,
    };

    for (usize i = reset_thread_data->cluster_begin; i < reset_thread_data->cluster_end; ++i) {
        for (usize j = 0; j < ENTRY_CLUSTER_SIZE; ++j) {
            reset_thread_data->tt->table[i].cluster_entry[j] = empty_entry;
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

TranspositionEntry *tt_probe(TranspositionTable *tt, Key key, bool *found) {
    TranspositionEntry *cluster_start = tt_entry_at(tt, key);

    // Try to find an entry matching the given key.
    for (usize i = 0; i < ENTRY_CLUSTER_SIZE; ++i) {
        TranspositionEntry *cur_entry = &cluster_start[i];

        if (!cur_entry->key || cur_entry->key == key) {
            // Refresh the generation counter to prevent it from being cleared.
            cur_entry->genbound = (u8)(tt->generation | (cur_entry->genbound & ~GENERATION_MASK));
            *found = (cur_entry->key == key);

            return cur_entry;
        }
    }

    TranspositionEntry *replace = cluster_start;

    // Find the slot with the minimal (depth + generation * 4) score to be replaced.
    for (usize i = 1; i < ENTRY_CLUSTER_SIZE; ++i) {
        TranspositionEntry *cur_entry = &cluster_start[i];

        if (tt_entry_replace_score(replace, tt->generation) > tt_entry_replace_score(cur_entry, tt->generation)) {
            replace = cur_entry;
        }
    }

    *found = false;
    return replace;
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
    if (bestmove != NO_MOVE || key != tt_entry->key) {
        tt_entry->bestmove = bestmove;
    }

    // Do not erase entries with high depth for the same position.
    if (bound == EXACT_BOUND || key != tt_entry->key || depth + 4 >= (i16)tt_entry->depth) {
        tt_entry->key = key;
        tt_entry->score = score;
        tt_entry->eval = eval;
        tt_entry->depth = depth;
        tt_entry->genbound = tt->generation | bound;
    }
}

u16 tt_hashfull(TranspositionTable *tt) {
    u16 count = 0;

    for (usize i = 0; i < 1000; ++i) {
        for (usize j = 0; j < ENTRY_CLUSTER_SIZE; ++j) {
            count += (tt->table[i].cluster_entry[j].genbound & GENERATION_MASK) == tt->generation;
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
