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

int tt_hashfull(void)
{
    extern transposition_t  g_hashtable;

    int count = 0;

    for (int i = 0; i < 1000; ++i)
        for (int j = 0; j < ClusterSize; ++j)
            count += (g_hashtable.table[i][j].genbound & 0xFC)
                == g_hashtable.generation;

    return (count / ClusterSize);
}
