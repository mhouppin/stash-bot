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

#include <stdio.h>
#include <stdlib.h>
#include "tt.h"

transposition_t TT = {
    0, NULL, 0
};

void    tt_resize(size_t mbsize)
{
    if (TT.table)
        free(TT.table);

    TT.cluster_count = mbsize * 1024 * 1024 / sizeof(cluster_t);
    TT.table = malloc(TT.cluster_count * sizeof(cluster_t));

    if (TT.table == NULL)
    {
        perror("Failed to allocate hashtable");
        exit(EXIT_FAILURE);
    }

    tt_bzero();
}
