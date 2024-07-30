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

#ifndef CORE_H
#define CORE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Workaround to avoid problems with duplicated function symbols,
// when -finline is disabled or when a function inlining fails.

#define INLINED static inline

// HINT() works as an assertion in debug builds, and as a compiler hint in
// optimized builds.
#ifdef NDEBUG
#define HINT(condition) if (!(condition)) __builtin_unreachable()
#else
#include <assert.h>
#define HINT(condition) assert(condition)
#endif

// Wrapper around u64 integer constants since the default type for integers is
// i32.
#define U64(x) UINT64_C(x)

// Basic type definitions.
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

#ifdef __SIZEOF_INT128__
#define HAS_INT128
typedef __int128 i128;
typedef unsigned __int128 u128;
#endif

// i8 API

INLINED u8 i8_abs(i8 value) { return value < 0 ? -value : value; }

// i16 API

INLINED u16 i16_abs(i16 value) { return value < 0 ? -value : value; }

// i32 API

INLINED i32 i32_min(i32 lhs, i32 rhs) { return lhs < rhs ? lhs : rhs; }

INLINED i32 i32_max(i32 lhs, i32 rhs) { return lhs > rhs ? lhs : rhs; }

INLINED i32 i32_clamp(i32 value, i32 lower, i32 upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

INLINED u16 i32_abs(i16 value) { return value < 0 ? -value : value; }

// i64 API

INLINED i64 i64_min(i64 lhs, i64 rhs) { return lhs < rhs ? lhs : rhs; }

INLINED i64 i64_max(i64 lhs, i64 rhs) { return lhs > rhs ? lhs : rhs; }

INLINED i64 i64_clamp(i64 value, i64 lower, i64 upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// isize API

INLINED isize isize_min(isize lhs, isize rhs) { return lhs < rhs ? lhs : rhs; }

INLINED isize isize_max(isize lhs, isize rhs) { return lhs > rhs ? lhs : rhs; }

INLINED isize isize_clamp(isize value, isize lower, isize upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u8 API

INLINED u8 u8_min(u8 lhs, u8 rhs) { return lhs < rhs ? lhs : rhs; }

INLINED u8 u8_max(u8 lhs, u8 rhs) { return lhs > rhs ? lhs : rhs; }

INLINED u8 u8_clamp(u8 value, u8 lower, u8 upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u32 API

INLINED u32 u32_min(u32 lhs, u32 rhs) { return lhs < rhs ? lhs : rhs; }

INLINED u32 u32_max(u32 lhs, u32 rhs) { return lhs > rhs ? lhs : rhs; }

INLINED u32 u32_clamp(u32 value, u32 lower, u32 upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u64 API

INLINED u64 u64_min(u64 lhs, u64 rhs) { return lhs < rhs ? lhs : rhs; }

INLINED u64 u64_max(u64 lhs, u64 rhs) { return lhs > rhs ? lhs : rhs; }

INLINED u64 u64_clamp(u64 value, u64 lower, u64 upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// usize API

INLINED usize usize_min(usize lhs, usize rhs) { return lhs < rhs ? lhs : rhs; }

INLINED usize usize_max(usize lhs, usize rhs) { return lhs > rhs ? lhs : rhs; }

INLINED usize usize_clamp(usize value, usize lower, usize upper)
{
    HINT(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// bitmanip API

INLINED u32 u64_count_ones(u64 value)
{
    return (u32)__builtin_popcountll(value);
}

INLINED u32 u64_count_zeros(u64 value)
{
    return u64_count_ones(~value);
}

INLINED u32 u64_first_one(u64 value)
{
    HINT(value != 0);
    return __builtin_ctzll(value);
}

INLINED u32 u64_first_zero(u64 value)
{
    HINT(~value != 0);
    return u64_first_one(~value);
}

INLINED u32 u64_last_one(u64 value)
{
    HINT(value != 0);
    return 63 - __builtin_clzll(value);
}

INLINED u32 u64_last_zero(u64 value)
{
    HINT(~value != 0);
    return u64_last_one(~value);
}

INLINED u32 u64_trailing_zeros(u64 value)
{
    return value == 0 ? 64 : u64_first_one(value);
}

INLINED u32 u64_trailing_ones(u64 value)
{
    return u64_trailing_zeros(~value);
}

INLINED u32 u64_leading_zeros(u64 value)
{
    return value == 0 ? 64 : __builtin_clzll(value);
}

INLINED u32 u64_leading_ones(u64 value)
{
    return u64_leading_zeros(~value);
}

INLINED u64 u64_flip_bytes(u64 value)
{
    return __builtin_bswap64(value);
}

INLINED u64 u64_flip_bits(u64 value)
{
    const u64 lo4 = U64(0x0F0F0F0F0F0F0F0F); // 0b00001111 x 8
    const u64 lo2 = U64(0x3333333333333333); // 0b00110011 x 8
    const u64 lo1 = U64(0x5555555555555555); // 0b01010101 x 8
    value = u64_flip_bytes(value);
    value = ((value & lo4) << 4) | ((value >> 4) & lo4);
    value = ((value & lo2) << 2) | ((value >> 2) & lo2);
    value = ((value & lo1) << 1) | ((value >> 1) & lo1);
    return value;
}

// Places the given address in cache
INLINED void prefetch(void *ptr)
{
    __builtin_prefetch(ptr);
}

#endif
