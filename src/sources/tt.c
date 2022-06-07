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

#include "tt.h"
#include "uci.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

transposition_t TT = {0, NULL, 0};

typedef struct tt_thread_s
{
    size_t start;
    size_t end;
    pthread_t thread;
} tt_thread_t;

void *tt_bzero_thread(void *data)
{
    tt_thread_t *threadData = data;
    const tt_entry_t zeroEntry = {0, NO_SCORE, NO_SCORE, 0, 0, NO_MOVE};

    for (size_t i = threadData->start; i < threadData->end; ++i)
        for (size_t j = 0; j < ClusterSize; ++j) TT.table[i].clEntry[j] = zeroEntry;

    return (NULL);
}

void tt_bzero(size_t threadCount)
{
    if (threadCount == 0)
    {
        fputs("Unable to zero TT: thread count equals zero\n", stderr);
        exit(EXIT_FAILURE);
    }

    tt_thread_t *threadList = malloc(sizeof(tt_thread_t) * threadCount);

    if (threadList == NULL)
    {
        perror("Unable to zero TT");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < threadCount; ++i)
    {
        threadList[i].start = TT.clusterCount * i / threadCount;
        threadList[i].end = TT.clusterCount * (i + 1) / threadCount;
    }

    for (size_t i = 1; i < threadCount; ++i)
        if (pthread_create(&threadList[i].thread, NULL, &tt_bzero_thread, &threadList[i]))
        {
            perror("Unable to zero TT");
            exit(EXIT_FAILURE);
        }

    tt_bzero_thread(&threadList[0]);

    for (size_t i = 1; i < threadCount; ++i) pthread_join(threadList[i].thread, NULL);

    free(threadList);
}

int tt_hashfull(void)
{
    int count = 0;

    for (int i = 0; i < 1000; ++i)
        for (int j = 0; j < ClusterSize; ++j)
            count += (TT.table[i].clEntry[j].genbound & 0xFC) == TT.generation;

    return (count / ClusterSize);
}

void tt_resize(size_t mbsize)
{
    if (TT.table) free(TT.table);

    TT.clusterCount = mbsize * 1024 * 1024 / sizeof(cluster_t);
    TT.table = malloc(TT.clusterCount * sizeof(cluster_t));

    if (TT.table == NULL)
    {
        perror("Failed to allocate hashtable");
        exit(EXIT_FAILURE);
    }

    tt_bzero((size_t)Options.threads);
}

tt_entry_t *tt_probe(hashkey_t key, bool *found)
{
    tt_entry_t *entry = tt_entry_at(key);

    for (int i = 0; i < ClusterSize; ++i)
        if (!entry[i].key || entry[i].key == key)
        {
            entry[i].genbound = (uint8_t)(TT.generation | (entry[i].genbound & 0x3));
            *found = (bool)entry[i].key;
            return (entry + i);
        }

    tt_entry_t *replace = entry;

    for (int i = 1; i < ClusterSize; ++i)
        if (replace->depth - ((259 + TT.generation - replace->genbound) & 0xFC)
            > entry[i].depth - ((259 + TT.generation - entry[i].genbound) & 0xFC))
            replace = entry + i;

    *found = false;
    return (replace);
}

void tt_save(tt_entry_t *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m)
{
    if (m || k != entry->key) entry->bestmove = (uint16_t)m;

    // Do not erase entries with higher depth for same position.

    if (b == EXACT_BOUND || k != entry->key || d + 4 >= entry->depth)
    {
        entry->key = k;
        entry->score = s;
        entry->eval = e;
        entry->genbound = TT.generation | (uint8_t)b;
        entry->depth = d;
    }
}
