/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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
typedef struct _Searchstack
{
    int plies;
    int doubleExtensions;
    score_t staticEval;
    move_t killers[2];
    move_t excludedMove;
    move_t currentMove;
    move_t *pv;
    piece_history_t *pieceHistory;
} Searchstack;

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

// Updates the continuation histories for the given piece-to combination.
void update_cont_histories(Searchstack *ss, int depth, piece_t piece, square_t to, bool failHigh);

// Updates the quiet history for the bestmove and all failed quiets.
void update_quiet_history(const Board *board, int depth, move_t bestmove, const move_t quiets[64],
    int qcount, Searchstack *ss);

// Updates the capture history for the bestmove and all failed captures.
void update_capture_history(const Board *board, int depth, move_t bestmove,
    const move_t captures[64], int ccount, Searchstack *ss);

// Quiescence search.
score_t qsearch(bool pvNode, Board *board, score_t alpha, score_t beta, Searchstack *ss);

// Standard search.
score_t search(bool pvNode, Board *board, int depth, score_t alpha, score_t beta, Searchstack *ss);

#endif
