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

#ifndef ENDGAME_H
# define ENDGAME_H

# include "board.h"

enum
{
    EGTB_Size = 8192
};

typedef score_t (*eg_func_t)(const board_t *, color_t);

typedef struct
{
    hashkey_t   key;
    eg_func_t   eval;
    color_t     winning;
}
endgame_entry_t;

extern endgame_entry_t  EndgameTable[EGTB_Size];

void    init_endgame_table(void);

score_t eval_draw(const board_t *board, color_t winning);
score_t eval_likely_draw(const board_t *board, color_t winning);
score_t eval_tricky_draw(const board_t *board, color_t winning);
score_t eval_kbnk(const board_t *board, color_t winning);

endgame_entry_t *endgame_probe(const board_t *board);

#endif
