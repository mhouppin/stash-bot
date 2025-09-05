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

#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Workaround to avoid problems with duplicated function symbols,
// when -finline is disabled or when a function inlining fails.

#define INLINED static inline

// Wrapper around u64 integer constants since the default type for integers is
// i32.
#define U64(x) UINT64_C(x)

#if (SIZE_MAX == UINT64_MAX)
#define FORMAT_LARGE_INT "%" PRIu64
typedef uint64_t LargeInt;
#define MAX_HASH 33554432
#else
#define FORMAT_LARGE_INT "%" PRIu32
typedef uint32_t LargeInt;
#define MAX_HASH 2048
#endif

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

typedef float f32;
typedef double f64;

#ifdef __SIZEOF_INT128__
#define HAS_INT128
typedef __int128 i128;
typedef unsigned __int128 u128;
#endif

// i8 API

INLINED u8 i8_abs(i8 value) {
    return value < 0 ? -value : value;
}

// i16 API

INLINED i16 i16_min(i16 lhs, i16 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED i16 i16_max(i16 lhs, i16 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED i16 i16_clamp(i16 value, i16 lower, i16 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

INLINED u16 i16_abs(i16 value) {
    return value < 0 ? -value : value;
}

// i32 API

INLINED i32 i32_min(i32 lhs, i32 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED i32 i32_max(i32 lhs, i32 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED i32 i32_clamp(i32 value, i32 lower, i32 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

INLINED u32 i32_abs(i32 value) {
    return value < 0 ? -value : value;
}

INLINED i32 i32_div_round(i32 value, i32 div) {
    assert(div != 0);

    const u32 v = i32_abs(value);
    const u32 d = i32_abs(div);
    const bool neg = (value < 0) ^ (div < 0);
    const u32 half = (d + 1) / 2;
    const u32 result = (v / d) + (v % d >= half);

    return neg ? -(i32)result : (i32)result;
}

// i64 API

INLINED i64 i64_min(i64 lhs, i64 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED i64 i64_max(i64 lhs, i64 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED i64 i64_clamp(i64 value, i64 lower, i64 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// isize API

INLINED isize isize_min(isize lhs, isize rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED isize isize_max(isize lhs, isize rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED isize isize_clamp(isize value, isize lower, isize upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u8 API

INLINED u8 u8_min(u8 lhs, u8 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED u8 u8_max(u8 lhs, u8 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED u8 u8_clamp(u8 value, u8 lower, u8 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u16 API

INLINED u16 u16_min(u16 lhs, u16 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED u16 u16_max(u16 lhs, u16 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED u16 u16_clamp(u16 value, u16 lower, u16 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u32 API

INLINED u32 u32_min(u32 lhs, u32 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED u32 u32_max(u32 lhs, u32 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED u32 u32_clamp(u32 value, u32 lower, u32 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// u64 API

INLINED u64 u64_min(u64 lhs, u64 rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED u64 u64_max(u64 lhs, u64 rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED u64 u64_clamp(u64 value, u64 lower, u64 upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

// usize API

INLINED usize usize_min(usize lhs, usize rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED usize usize_max(usize lhs, usize rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED usize usize_clamp(usize value, usize lower, usize upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

INLINED usize usize_next_multiple_of(usize value, usize divisor) {
    assert(divisor != 0);
    value += divisor - 1;
    return value - (value % divisor);
}

INLINED usize usize_div_ceil(usize value, usize divisor) {
    assert(divisor != 0);
    return value / divisor + (value % divisor != 0);
}

// bitmanip API

INLINED u32 u64_count_ones(u64 value) {
    return (u32)__builtin_popcountll(value);
}

INLINED u32 u64_count_zeros(u64 value) {
    return u64_count_ones(~value);
}

INLINED u32 u64_first_one(u64 value) {
    assert(value != 0);
    return __builtin_ctzll(value);
}

INLINED u32 u64_first_zero(u64 value) {
    assert(~value != 0);
    return u64_first_one(~value);
}

INLINED u32 u64_last_one(u64 value) {
    assert(value != 0);
    return 63 - __builtin_clzll(value);
}

INLINED u32 u64_last_zero(u64 value) {
    assert(~value != 0);
    return u64_last_one(~value);
}

INLINED u32 u64_trailing_zeros(u64 value) {
    return value == 0 ? 64 : u64_first_one(value);
}

INLINED u32 u64_trailing_ones(u64 value) {
    return u64_trailing_zeros(~value);
}

INLINED u32 u64_leading_zeros(u64 value) {
    return value == 0 ? 64 : __builtin_clzll(value);
}

INLINED u32 u64_leading_ones(u64 value) {
    return u64_leading_zeros(~value);
}

INLINED u64 u64_flip_bytes(u64 value) {
    return __builtin_bswap64(value);
}

INLINED u64 u64_flip_bits(u64 value) {
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
INLINED void prefetch(void *ptr) {
    __builtin_prefetch(ptr);
}

// Typedefs for time-related values, always described in milliseconds.
// Note that Duration also includes negative values.
typedef i64 Timepoint;
typedef i64 Duration;

// Returns the current timepoint
Timepoint timepoint_now(void);

// Returns the time elapsed between two timepoints
INLINED Duration timepoint_diff(Timepoint from, Timepoint to) {
    return (Duration)(to - from);
}

INLINED Duration duration_min(Duration lhs, Duration rhs) {
    return lhs < rhs ? lhs : rhs;
}

INLINED Duration duration_max(Duration lhs, Duration rhs) {
    return lhs > rhs ? lhs : rhs;
}

INLINED Duration duration_clamp(Duration value, Duration lower, Duration upper) {
    assert(lower <= upper);
    return value < lower ? lower : value > upper ? upper : value;
}

#endif
