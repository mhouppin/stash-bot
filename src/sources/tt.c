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

#include "tt.h"
#include "uci.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

TranspositionTable SearchTT = {0, NULL, 0};

typedef struct _BzeroThread
{
    size_t start;
    size_t end;
    pthread_t thread;
} BzeroThread;

void *tt_bzero_thread(void *data)
{
    BzeroThread *threadData = data;
    const TT_Entry zeroEntry = {0, NO_SCORE, NO_SCORE, 0, 0, NO_MOVE};

    for (size_t i = threadData->start; i < threadData->end; ++i)
        for (size_t j = 0; j < ClusterSize; ++j) SearchTT.table[i].clEntry[j] = zeroEntry;

    return (NULL);
}

void tt_bzero(size_t threadCount)
{
    // Guard against thread count being zero.
    if (threadCount == 0)
    {
        fputs("Unable to zero TT: thread count equals zero\n", stderr);
        exit(EXIT_FAILURE);
    }

    // Allocate the list of helper threads for zeroing the TT.
    BzeroThread *threadList = malloc(sizeof(BzeroThread) * threadCount);

    if (threadList == NULL)
    {
        perror("Unable to zero TT");
        exit(EXIT_FAILURE);
    }

    // Define each thread's work range.
    for (size_t i = 0; i < threadCount; ++i)
    {
        threadList[i].start = SearchTT.clusterCount * i / threadCount;
        threadList[i].end = SearchTT.clusterCount * (i + 1) / threadCount;
    }

    // Create all helper threads.
    for (size_t i = 1; i < threadCount; ++i)
        if (pthread_create(&threadList[i].thread, NULL, &tt_bzero_thread, &threadList[i]))
        {
            perror("Unable to zero TT");
            exit(EXIT_FAILURE);
        }

    // Recycle the main thread for zeroing.
    tt_bzero_thread(&threadList[0]);

    // Wait for all helper threads.
    for (size_t i = 1; i < threadCount; ++i) pthread_join(threadList[i].thread, NULL);

    free(threadList);
}

int tt_hashfull(void)
{
    int count = 0;

    for (int i = 0; i < 1000; ++i)
        for (int j = 0; j < ClusterSize; ++j)
            count += (SearchTT.table[i].clEntry[j].genbound & 0xFC) == SearchTT.generation;

    return (count / ClusterSize);
}

void tt_resize(size_t mbsize)
{
    // Free the old TT if it exists.
    if (SearchTT.table) free(SearchTT.table);

    SearchTT.clusterCount = mbsize * 1024 * 1024 / sizeof(TT_Cluster);
    SearchTT.table = malloc(SearchTT.clusterCount * sizeof(TT_Cluster));

    // Check if the TT allocation went correctly.
    if (SearchTT.table == NULL)
    {
        perror("Failed to allocate hashtable");
        exit(EXIT_FAILURE);
    }

    // Reset the new TT.
    tt_bzero((size_t)UciOptionFields.threads);
}

TT_Entry *tt_probe(hashkey_t key, bool *found)
{
    TT_Entry *entry = tt_entry_at(key);

    // Try to find an entry matching the given key.
    for (int i = 0; i < ClusterSize; ++i)
        if (!entry[i].key || entry[i].key == key)
        {
            // Refresh the generation counter to prevent it from being cleared.
            entry[i].genbound = (uint8_t)(SearchTT.generation | (entry[i].genbound & 0x3));
            *found = (bool)entry[i].key;
            return (entry + i);
        }

    TT_Entry *replace = entry;

    // Find the slot with the minimal (depth + generation * 4) score.
    for (int i = 1; i < ClusterSize; ++i)
        if (replace->depth - ((259 + SearchTT.generation - replace->genbound) & 0xFC)
            > entry[i].depth - ((259 + SearchTT.generation - entry[i].genbound) & 0xFC))
            replace = entry + i;

    *found = false;
    return (replace);
}

void tt_save(TT_Entry *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m)
{
    if (m || k != entry->key) entry->bestmove = (uint16_t)m;

    // Do not erase entries with high depth for the same position.
    if (b == EXACT_BOUND || k != entry->key || d + 4 >= entry->depth)
    {
        entry->key = k;
        entry->score = s;
        entry->eval = e;
        entry->genbound = SearchTT.generation | (uint8_t)b;
        entry->depth = d;
    }
}
