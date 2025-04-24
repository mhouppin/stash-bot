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

#ifndef WDL_H
#define WDL_H

#include "chess_types.h"
#include "core.h"

// The structure that holds the parameters for WDL estimation.
// See https://github.com/official-stockfish/WDL_model
typedef struct {
    f64 mean;
    f64 spread;
} WdlParams;

// The structure that holds the WDL estimation
typedef struct {
    u16 win;
    u16 draw;
    u16 loss;
} WdlValue;

// Initializes the WDL params with the given material distribution
void wdl_params_init(WdlParams *wdl_params, u32 material);

// Return the expected winrate for a given score, in per mille units
u16 wdl_params_get_expected_winrate(const WdlParams *wdl_params, Score score);

// Initializes the WDL expected distribution with the given material and score
void wdl_value_init(WdlValue *wdl_value, Score score, u32 material);

// Returns the score, normalized so that a +100cp advantage with a material count of 58 units on the
// board corresponds to an expected winrate of 50%
Score normalized_score(Score score);

#endif
