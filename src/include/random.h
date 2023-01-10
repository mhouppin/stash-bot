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

#ifndef RANDOM_H
#define RANDOM_H

#include "types.h"

// Generates a random 64-bit unsigned integer based on the given seed using Xorshift.
INLINED uint64_t qrandom(uint64_t *seed)
{
    uint64_t x = *seed;

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *seed = x;

    return (x * UINT64_C(0x2545F4914F6CDD1D));
}

#endif // RANDOM_H
