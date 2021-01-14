/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

tt_entry_t  *tt_probe(hashkey_t key, bool *found)
{
    extern transposition_t  g_hashtable;

    tt_entry_t  *entry = tt_entry_at(key);

    for (int i = 0; i < ClusterSize; ++i)
        if (!entry[i].key || entry[i].key == key)
        {
            entry[i].genbound = (uint8_t)(g_hashtable.generation | (entry[i].genbound & 0x3));
            *found = (bool)entry[i].key;
            return (entry + i);
        }

    tt_entry_t  *replace = entry;

    for (int i = 1; i < ClusterSize; ++i)
        if (replace->depth
                - ((259 + g_hashtable.generation - replace->genbound) & 0xFC)
            > entry[i].depth
                - ((259 + g_hashtable.generation - entry[i].genbound) & 0xFC))
            replace = entry + i;

    *found = false;
    return (replace);
}
