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

#include "movelist.h"
#include <string.h>

void    place_top_move(extmove_t *begin, extmove_t *end)
{
    extmove_t   *top = begin;

    for (extmove_t *i = begin + 1; i < end; ++i)
        if (i->score > top->score)
            top = i;

    extmove_t   tmp = *top;
    for (extmove_t *i = top; i > begin; --i)
        *i = *(i - 1);
    *begin = tmp;
}
