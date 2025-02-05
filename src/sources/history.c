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

#include "history.h"

static i64 saturating_trinomial(i64 x, i64 a, i64 b, i64 c, i64 maxval) {
    return i64_min(a * x * x + b * x + c, maxval);
}

i16 butterfly_hist_bonus(u16 depth) {
    return saturating_trinomial(depth, 26, -25, -31, 2567);
}

i16 conthist_bonus(u16 depth) {
    return saturating_trinomial(depth, 32, -10, -41, 2330);
}

i16 capture_hist_bonus(u16 depth) {
    return saturating_trinomial(depth, 25, -9, 4, 2402);
}
