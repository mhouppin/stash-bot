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

#include "engine.h"

INLINED int rtm_greater_than(root_move_t *right, root_move_t *left)
{
    if (right->score != left->score)
        return (right->score > left->score);
    else
        return (right->previous_score > left->previous_score);
}

void        sort_root_moves(root_move_t *begin, root_move_t *end)
{
    const int   size = (int)(end - begin);

    for (int i = 1; i < size; ++i)
    {
        root_move_t tmp = begin[i];
        int         j = i - 1;
        while (j >= 0 && rtm_greater_than(&tmp, begin + j))
        {
            begin[j + 1] = begin[j];
            --j;
        }
        begin[j + 1] = tmp;
    }
}

root_move_t *find_root_move(root_move_t *begin, root_move_t *end, move_t move)
{
    while (begin < end)
    {
        if (begin->move == move)
            return (begin);
        ++begin;
    }
    return (NULL);
}
