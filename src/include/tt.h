/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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
# define TT_H

# include <string.h>
# include "score.h"
# include "hashkey.h"
# include "move.h"

typedef struct
{
    hashkey_t   key;
    score_t     score;
    score_t     eval;
    uint8_t     depth;
    uint8_t     genbound;
    uint16_t    bestmove;
}        tt_entry_t;

enum
{
    ClusterSize = 4
};

typedef tt_entry_t  cluster_t[ClusterSize];

typedef struct
{
    size_t      cluster_count;
    cluster_t   *table;
    uint8_t     generation;
}        transposition_t;

INLINED tt_entry_t  *tt_entry_at(hashkey_t k)
{
    extern transposition_t  g_hashtable;

    return (g_hashtable.table[mul_hi64(k, g_hashtable.cluster_count)]);
}

INLINED void        tt_clear(void)
{
    extern transposition_t  g_hashtable;

    g_hashtable.generation += 4;
}

INLINED void        tt_bzero(void)
{
    extern transposition_t  g_hashtable;

    memset(g_hashtable.table, 0, sizeof(cluster_t) * g_hashtable.cluster_count);
}

INLINED score_t     score_to_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s + plies : s <= -MATE_FOUND ? s - plies : s);
}

INLINED score_t     score_from_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s - plies : s <= -MATE_FOUND ? s + plies : s);
}

tt_entry_t  *tt_probe(hashkey_t key, bool *found);
void        tt_save(tt_entry_t *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m);
int         tt_hashfull(void);
void        tt_resize(size_t mbsize);

#endif
