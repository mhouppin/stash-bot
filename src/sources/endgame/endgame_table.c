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

#include "endgame.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

endgame_entry_t EndgameTable[EGTB_SIZE];

void    add_colored_entry(color_t winning_side, char *wpieces, char *bpieces, endgame_func_t func)
{
    char    bcopy[16];
    char    fen[128];

    strcpy(bcopy, bpieces);

    for (size_t i = 0; bcopy[i]; ++i)
        bcopy[i] = tolower(bcopy[i]);

    sprintf(fen, "8/%s%d/8/8/8/8/%s%d/8 w - - 0 1", bcopy, 8 - (int)strlen(bcopy),
        wpieces, 8 - (int)strlen(wpieces));

    board_t         board;
    boardstack_t    stack;

    set_board(&board, fen, false, &stack);

    endgame_entry_t *entry = &EndgameTable[board.stack->material_key & (EGTB_SIZE - 1)];

    if (entry->key)
    {
        fputs("Error: key conflicts in endgame table\n", stderr);
        exit(EXIT_FAILURE);
    }
    entry->key = board.stack->material_key;
    entry->func = func;
    entry->winning_side = winning_side;
}

void    add_endgame_entry(const char *pieces, endgame_func_t eval)
{
    char    buf[32];

    strcpy(buf, pieces);

    char    *split = strchr(buf, 'v');
    *split = '\0';

    add_colored_entry(WHITE, buf, split + 1, eval);
    add_colored_entry(BLACK, split + 1, buf, eval);
}

void    init_endgame_table(void)
{
    memset(EndgameTable, 0, sizeof(EndgameTable));

    add_colored_entry(WHITE, "K", "K", &eval_draw);

    // 3-man endgames
    add_endgame_entry("KPvK", &eval_kpk);
    add_endgame_entry("KNvK", &eval_draw);
    add_endgame_entry("KBvK", &eval_draw);

    // 4-man endgames
    add_endgame_entry("KNNvK", &eval_draw);
    add_endgame_entry("KBvKN", &eval_draw);
    add_endgame_entry("KRvKP", &eval_krkp);
    add_endgame_entry("KRvKN", &eval_krkn);
    add_endgame_entry("KRvKB", &eval_krkb);
    add_colored_entry(WHITE, "KN", "KN", &eval_draw);
    add_colored_entry(WHITE, "KB", "KB", &eval_draw);
    add_endgame_entry("KQvKR", &eval_kqkr);
    add_endgame_entry("KQvKP", &eval_kqkp);

    // 5-man endgames
    add_endgame_entry("KBBvKB", &eval_draw);
    add_endgame_entry("KNNvKP", &eval_knnkp);

    add_endgame_entry("KBNvK", &eval_kbnk);
}

const endgame_entry_t *endgame_probe(const board_t *board)
{
    endgame_entry_t *entry = &EndgameTable[board->stack->material_key & (EGTB_SIZE - 1)];

    return (entry->key == board->stack->material_key ? entry : NULL);
}