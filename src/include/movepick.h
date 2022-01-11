/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#ifndef MOVEPICK_H
# define MOVEPICK_H

# include "lazy_smp.h"
# include "movelist.h"

typedef struct
{
    int plies;
    score_t staticEval;
    move_t killers[2];
    move_t excludedMove;
    move_t currentMove;
    move_t *pv;
    piece_history_t *pieceHistory;
}
searchstack_t;

enum
{
    PICK_TT,
    GEN_INSTABLE,
    PICK_GOOD_INSTABLE,
    PICK_KILLER1,
    PICK_KILLER2,
    PICK_COUNTER,
    GEN_QUIET,
    PICK_QUIET,
    PICK_BAD_INSTABLE,

    CHECK_PICK_TT,
    CHECK_GEN_ALL,
    CHECK_PICK_ALL
};

typedef struct movepick_s
{
    movelist_t list;
    extmove_t *cur, *badCaptures;
    bool inQsearch;
    int stage;
    move_t ttMove;
    move_t killer1;
    move_t killer2;
    move_t counter;
    const board_t *board;
    const worker_t *worker;
    piece_history_t *pieceHistory[2];
}
movepick_t;

void movepick_init(movepick_t *mp, bool inQsearch, const board_t *board,
    const worker_t *worker, move_t ttMove, searchstack_t *ss);

move_t  movepick_next_move(movepick_t *mp, bool skipQuiets);

#endif
