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

#include <stdlib.h>
#include "board.h"

boardstack_t    *boardstack_dup(const boardstack_t *stack)
{
    if (!stack)
        return (NULL);

    boardstack_t    *new_stack = malloc(sizeof(boardstack_t));

    *new_stack = *stack;
    new_stack->prev = boardstack_dup(stack->prev);
    return (new_stack);
}

void    boardstack_free(boardstack_t *stack)
{
    while (stack)
    {
        boardstack_t    *next = stack->prev;
        free(stack);
        stack = next;
    }
}
