/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#ifndef SEARCH_CONFIG_H
#define SEARCH_CONFIG_H

#include "core.h"
#include "movelist.h"
#include "strview.h"

typedef struct {
    i64 move_overhead;
    i64 multi_pv;
    bool show_wdl;
    bool normalize_score;

    Duration wtime;
    Duration btime;
    Duration winc;
    Duration binc;
    u16 movestogo;
    bool tc_is_set;

    u16 depth;
    u64 nodes;
    Duration movetime;
    u16 mate;
    u16 perft;
    bool infinite;
    bool ponder;
    Movelist searchmoves;
} SearchParams;

// Initializes the search params with the given UCI options
void search_params_init(
    SearchParams *search_params,
    i64 move_overhead,
    i64 multi_pv,
    bool show_wdl,
    bool normalize_score
);

// Sets the search params according to the given UCI command
void search_params_set_from_uci(
    SearchParams *restrict search_params,
    const Board *restrict root_board,
    StringView command_args
);

// Copies the search params to a new struct
void search_params_copy(SearchParams *restrict search_params, const SearchParams *restrict other);

#endif
