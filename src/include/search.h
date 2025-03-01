/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include "worker.h"

void search_init(void);

// Struct for holding search data
typedef struct {
    i16 plies;
    i16 double_extensions;
    Score static_eval;
    Move killers[2];
    Move excluded_move;
    Move current_move;
    PvLine pv;
    PieceHistory *piece_history;
} Searchstack;

void searchstack_init(Searchstack *ss);

void main_worker_search(Worker *worker);

void worker_search(Worker *worker);

// Searches the PV line index indicated by worker->pv_line. Returns true if the search can continue,
// and false if the search must abort now
bool worker_search_pv_line(Worker *worker, u16 depth, u16 multi_pv, Searchstack *ss);

Score search(
    bool pv_node,
    Board *board,
    i16 depth,
    Score alpha,
    Score beta,
    Searchstack *ss,
    bool cut_node
);

Score qsearch(bool pv_node, Board *board, Score alpha, Score beta, Searchstack *ss);

void update_continuation_histories(
    Searchstack *ss,
    i16 depth,
    Piece piece,
    Square to,
    bool fail_high
);

void update_quiet_history(
    const Board *board,
    i16 depth,
    Move bestmove,
    const Move *tried_quiets,
    i16 quiet_count,
    Searchstack *ss
);

void update_capture_history(
    const Board *board,
    i16 depth,
    Move bestmove,
    const Move *tried_noisy,
    i16 noisy_count,
    Searchstack *ss
);

#endif
