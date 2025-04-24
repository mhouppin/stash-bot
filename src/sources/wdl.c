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

#include "wdl.h"

#include <math.h>

void wdl_params_init(WdlParams *wdl_params, u32 material) {
    // clang-format off
    static const f64 as[4] = {-115.80269028,  326.13955902, -411.17611305,  342.29869813};
    static const f64 bs[4] = { -35.81090243,   83.17183837,  -52.14133486,   81.73401953};
    // clang-format on

    // The fitted model only uses data for material counts in [17, 78], and is
    // anchored at count 58.
    f64 m = u32_clamp(material, 17, 78) / 58.0;

    // Return a = p_a(material) and b = p_b(material), see
    // https://github.com/official-stockfish/WDL_model
    wdl_params->mean = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    wdl_params->spread = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];
}

u16 wdl_params_get_expected_winrate(const WdlParams *wdl_params, Score score) {
    return (u16)(0.5 + 1000.0 / (1.0 + exp((wdl_params->mean - score) / wdl_params->spread)));
}

void wdl_value_init(WdlValue *wdl_value, Score score, u32 material) {
    WdlParams wdl_params;

    wdl_params_init(&wdl_params, material);
    wdl_value->win = wdl_params_get_expected_winrate(&wdl_params, score);
    wdl_value->loss = wdl_params_get_expected_winrate(&wdl_params, -score);
    wdl_value->draw = 1000 - wdl_value->win - wdl_value->loss;
}

Score normalized_score(Score score) {
    static const Score NormalizeScore = 141;

    return score_is_mate(score) ? score : (i32)score * 100 / NormalizeScore;
}
