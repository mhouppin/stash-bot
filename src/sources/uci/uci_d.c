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

#include <stdio.h>
#include "board.h"
#include "engine.h"

void    uci_d(const char *args)
{
    (void)args;
    extern const board_t    g_board;
    const char              *grid = "+---+---+---+---+---+---+---+---+";
    const char              *piece_to_char = " PNBRQK  pnbrqk";

    puts(grid);

    for (file_t rank = RANK_8; rank >= RANK_1; --rank)
    {
        for (file_t file = FILE_A; file <= FILE_H; ++file)
            printf("| %c ", piece_to_char[piece_on(&g_board,
                create_square(file, rank))]);

        puts("|");
        puts(grid);
    }

    printf("\nKey: 0x%lx\n", (unsigned long)g_board.stack->board_key);

    double  eval = (double)evaluate(&g_board) / 100.0;
    printf("Eval: %+.2lf\n\n", g_board.side_to_move == WHITE ? eval : -eval);
    
    fflush(stdout);
}
