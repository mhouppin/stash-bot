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

INLINED i16 history_bonus(u16 depth) {
    const i32 d = (i32)depth;

    return (i16)i32_min(27 * d * d - 32 * d - 34, 2461);
}

static i64 saturating_trinomial(i64 x, i64 a, i64 b, i64 c, i64 maxval) {
    return i64_min(a * x * x + b * x + c, maxval);
}

i64 BtflHistX0 = -34;
i64 BtflHistX1 = -32;
i64 BtflHistX2 = 27;
i64 BtflHistMax = 2461;

i16 butterfly_hist_bonus(u16 depth) {
    return saturating_trinomial(depth, BtflHistX2, BtflHistX1, BtflHistX0, BtflHistMax);
}

i64 ContHistX0 = -34;
i64 ContHistX1 = -32;
i64 ContHistX2 = 27;
i64 ContHistMax = 2461;

i16 conthist_bonus(u16 depth) {
    return saturating_trinomial(depth, ContHistX2, ContHistX1, ContHistX0, ContHistMax);
}

i64 CaptHistX0 = -34;
i64 CaptHistX1 = -32;
i64 CaptHistX2 = 27;
i64 CaptHistMax = 2461;

i16 capture_hist_bonus(u16 depth) {
    return saturating_trinomial(depth, CaptHistX2, CaptHistX1, CaptHistX0, CaptHistMax);
}
