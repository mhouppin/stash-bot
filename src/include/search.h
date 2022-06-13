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

#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "history.h"

// Struct for holding search data
typedef struct
{
    int plies;
    score_t staticEval;
    move_t killers[2];
    move_t excludedMove;
    move_t currentMove;
    move_t *pv;
    piece_history_t *pieceHistory;
} searchstack_t;

// Global for Late Move Reductions
extern int Reductions[64][64];

// Global for Late Move Pruning
extern int Pruning[2][7];

enum
{
    MAX_PLIES = 240
};

// Initializes the search tables.
void init_search_tables(void);

// Updates the quiet history for the bestmove and all failed quiets.
void update_quiet_history(const board_t *board, int depth, move_t bestmove, const move_t quiets[64],
    int qcount, searchstack_t *ss);

// Updates the capture history for the bestmove and all failed captures.
void update_capture_history(const board_t *board, int depth, move_t bestmove,
    const move_t captures[64], int ccount, searchstack_t *ss);

// Quiescence search.
score_t qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode);

// Standard search.
score_t search(
    board_t *board, int depth, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode);

#endif