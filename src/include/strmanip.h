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

#ifndef STRMANIP_H
#define STRMANIP_H

#include "core.h"
#include "strview.h"
#include <string.h>

typedef struct {
    u8 *data;
    usize size;
} StrHeapBuffer;

typedef struct {
    usize capacity;

    // The core of the SSO (Small String Optimization):
    // - For capacity values > STRING_CAP_STACKALLOC, we store a pointer to the heap-allocated byte
    //   array holding the string, and the number of bytes stored in it (excluding the nullbyte).
    // - For capacity values <= STRING_CAP_STACKALLOC, we store the byte array holding the string
    //   directly in the struct, and the capacity value above refers to the number of bytes stored
    //   (still excluding the nullbyte).
    // An important thing to note is that the capacity value always excludes the extra terminating
    // nullbyte for storage, so the effective allocated space is always one byte larger than
    // `capacity`.
    union {
        StrHeapBuffer heap_buffer;
        u8 stack_buffer[sizeof(StrHeapBuffer)];
    };
} String;

static const usize STRING_MIN_ALLOC = sizeof(StrHeapBuffer);
static const usize STRING_CAP_STACKALLOC = STRING_MIN_ALLOC - 1;

// Tests if the underlying string is stored directly on the stack.
INLINED bool string_on_stack(const String *string) {
    return string->capacity <= STRING_CAP_STACKALLOC;
}

// Returns a pointer to the underlying null-terminated string.
INLINED u8 *string_data(String *string) {
    return string_on_stack(string) ? string->stack_buffer : string->heap_buffer.data;
}

// Returns a const pointer to the underlying null-terminated string.
INLINED const u8 *string_cdata(const String *string) {
    return string_on_stack(string) ? string->stack_buffer : string->heap_buffer.data;
}

// Returns the number of bytes in the string, not including the terminating nullbyte.
INLINED usize string_size(const String *string) {
    return string_on_stack(string) ? string->capacity : string->heap_buffer.size;
}

// Returns the current storage capacity of the string.
INLINED usize string_capacity(const String *string) {
    // Effective capacity is always at least as large as the stack buffer minus one.
    // Technically equivalent to `usize_max(string->capacity, STRING_CAP_STACKALLOC)`.
    return string_on_stack(string) ? STRING_CAP_STACKALLOC : string->capacity;
}

// Tells if the string is in a valid state. This is only meant to be used as a debugging facility.
INLINED bool string_is_valid(const String *string) {
    // Common potential points of failure when the user tries to modify the string object directly.
    return string_capacity(string) >= string_size(string)
        && string_cdata(string)[string_size(string)] == 0;
}

// Asserts that the string is in a valid state. This is only meant to be used as a debugging
// facility.
INLINED void string_sanitize(const String *string __attribute__((unused))) {
    assert(string_is_valid(string));
}

// Constructs a string view from the string.
INLINED StringView strview_from_string(const String *string) {
    return strview_from_raw_data(string_cdata(string), string_size(string));
}

// Initializes the string with no bytes stored in it.
void string_init(String *string);

// Initializes the string with the provided data buffer.
void string_init_from_raw_data(String *restrict string, const u8 *restrict data, usize size);

// Initializes the string with the provided null-terminated string.
void string_init_from_cstr(String *restrict string, const char *data);

// Initializes the string with the provided string view.
void string_init_from_strview(String *string, StringView other);

// Frees all memory associated with the string.
void string_destroy(String *string);

// Increases the capacity of the string to hold at least `new_capacity` bytes.
void string_reserve(String *string, usize new_capacity);

// Tries to shrink the capacity of the string to reduce its memory footprint.
// Note: this function is allowed to be a no-op.
void string_shrink_to_fit(String *string);

// Resets the size of the string to zero.
void string_clear(String *string);

// Inserts a byte in the string at the provided index.
void string_insert(String *string, usize index, u8 c);

// Inserts an array of bytes in the string at the provided index.
void string_insert_range(String *restrict string, usize index, const u8 *restrict data, usize size);

// Inserts a string view in the string at the provided index.
void string_insert_strview(String *string, usize index, StringView other);

// Adds a byte at the end of the string.
void string_push_back(String *string, u8 c);

// Adds an array of bytes at the end of the string.
void string_push_back_range(String *restrict string, const u8 *restrict data, usize size);

// Adds a string view at the end of the string.
void string_push_back_strview(String *string, StringView other);

// Adds a formatted i64 at the end of the string.
void string_push_back_i64(String *string, i64 value);

// Adds a formatted u64 at the end of the string.
void string_push_back_u64(String *string, u64 value);

// Removes the byte of the specified index.
void string_erase(String *restrict string, usize index);

// Removes the bytes in the specified range, with the one at the end index excluded.
void string_erase_range(String *restrict string, usize start, usize end);

// Removes the last byte of the string.
void string_pop_back(String *restrict string);

// Removes all the bytes of the string from the provided index up to the end.
void string_pop_back_range(String *restrict string, usize start);

// Replaces the byte at the provided index.
void string_replace(String *restrict string, usize index, u8 c);

// Replaces a range of bytes starting at the provided index with the given byte array.
void string_replace_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
);

// Replaces a range of bytes starting at the provided index with the given string view.
void string_replace_strview(String *string, usize index, StringView other);

#endif
