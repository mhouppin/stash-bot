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

#include "strmanip.h"

#include <stdlib.h>

#include "wmalloc.h"

static void string_add_nullbyte(String *string) {
    string_data(string)[string_size(string)] = 0;
}

static void string_increase_size(String *string, usize size) {
    if (string_on_stack(string)) {
        string->capacity += size;
    } else {
        string->heap_buffer.size += size;
    }
}

static void string_decrease_size(String *string, usize size) {
    if (string_on_stack(string)) {
        string->capacity -= size;
    } else {
        string->heap_buffer.size -= size;
    }
}

static void string_resize_stack_to_heap(String *string, usize new_capacity) {
    // We MUST NOT use heap allocation for capacity values smaller than the stack-allocated storage.
    // We also MUST NOT call this function with heap-allocated data in our string struct.
    assert(new_capacity > STRING_CAP_STACKALLOC);
    assert(string_on_stack(string));
    u8 *ptr;

    // We round the allocation to the nearest >= multiple of (sizeof(void *) * 2), since that will
    // be the minimal alignment required on almost every machine we run on, and the extra bytes are
    // almost always not reused by the malloc implementation.
    // With the extra nullbyte this effectively makes the min alloc be 32 bytes wide on 64-bit
    // systems, with a min heap capacity of 31 bytes.
    new_capacity = usize_next_multiple_of(new_capacity + 1, sizeof(void *) * 2);
    ptr = wrap_malloc(new_capacity);

    // Note that we do copy the nullbyte here, as the string MUST already be in a valid state at
    // this point, and it avoids us doing manual copies in functions that do not modify the data
    // state, such as string_reserve().
    memcpy(ptr, string->stack_buffer, string->capacity + 1);
    string->heap_buffer.data = ptr;
    string->heap_buffer.size = string->capacity;
    string->capacity = new_capacity - 1;
}

static void string_resize_heap_to_heap(String *string, usize new_capacity) {
    // We MUST NOT use heap allocation for capacity values smaller than the stack-allocated storage.
    // We also MUST NOT call this function with stack-allocated data in our string struct.
    assert(new_capacity > STRING_CAP_STACKALLOC);
    assert(!string_on_stack(string));

    // Same rounding as the function above. There's a slight difference compared to other resizing
    // functions, as this one can be used for both increasing AND shrinking the heap-allocated
    // buffer.
    new_capacity = usize_next_multiple_of(new_capacity + 1, sizeof(void *) * 2);
    string->heap_buffer.data = wrap_realloc(string->heap_buffer.data, new_capacity);
    string->capacity = new_capacity - 1;
}

static void string_shrink_heap_to_stack(String *string) {
    // We MUST NOT call this function with stack-allocated data in our string struct.
    // We also MUST NOT use stack allocation for string sizes larger than the stack-allocated
    // available storage.
    assert(!string_on_stack(string));
    assert(string->heap_buffer.size <= STRING_CAP_STACKALLOC);
    u8 tmp_buffer[sizeof(StrHeapBuffer)];

    // No capacity value was provided here, as we do not store an explicit capacity value on using
    // stack allocation. Copy the string data to a temporary buffer before erasing the heap-alloc
    // information.
    memcpy(tmp_buffer, string->heap_buffer.data, string->heap_buffer.size + 1);
    string->capacity = string->heap_buffer.size;
    free(string->heap_buffer.data);
    memcpy(string->stack_buffer, tmp_buffer, string->capacity + 1);
}

static void string_resize_if_needed(String *string, usize extra_bytes) {
    const usize min_new_size = string_size(string) + extra_bytes;

    // Delay heap allocations as much as possible.
    if (string_capacity(string) >= min_new_size) {
        return;
    }

    // We use exponential growth for preserving amortized constant/linear execution time on
    // insertion operations, but we stay on the conservative side by only increasing the buffer by
    // 50% of its original size if possible.
    string_reserve(string, usize_max(min_new_size, string_size(string) * 3 / 2));
}

void string_init(String *string) {
    string->capacity = 0;
    string->stack_buffer[0] = 0;
}

void string_init_from_raw_data(String *restrict string, const u8 *restrict data, usize size) {
    if (size <= STRING_CAP_STACKALLOC) {
        string->capacity = size;
        memcpy(string->stack_buffer, data, size);
    } else {
        string_init(string);
        string_resize_stack_to_heap(string, size);
        memcpy(string->heap_buffer.data, data, size);
    }

    string_add_nullbyte(string);
}

void string_init_from_cstr(String *restrict string, const char *data) {
    string_init_from_raw_data(string, (const u8 *)data, strlen(data));
}

void string_init_from_strview(String *string, StringView other) {
    string_init_from_raw_data(string, other.data, other.size);
}

void string_destroy(String *string) {
    if (!string_on_stack(string)) {
        free(string->heap_buffer.data);
    }

#ifndef NDEBUG
    // In debug mode, leave the string in an invalid state to have an easier time diagnosing
    // "use-after-free" errors.
    string->capacity = SIZE_MAX;
    string->heap_buffer.size = 0;
    string->heap_buffer.data = NULL;
#endif
}

void string_reserve(String *string, usize new_capacity) {
    string_sanitize(string);

    // Reserving less than what's already available is a no-op.
    if (new_capacity <= STRING_CAP_STACKALLOC || new_capacity <= string->capacity) {
        return;
    }

    // With the check above, we already know we're going to need a heap allocation.
    if (string_on_stack(string)) {
        string_resize_stack_to_heap(string, new_capacity);
    } else {
        string_resize_heap_to_heap(string, new_capacity);
    }
}

void string_shrink_to_fit(String *string) {
    string_sanitize(string);

    if (string_on_stack(string)) {
        return;
    }

    // We're happy to get rid of heap allocations if requested by the user.
    if (string_size(string) <= STRING_CAP_STACKALLOC) {
        string_shrink_heap_to_stack(string);
    } else {
        string_resize_heap_to_heap(string, string_size(string));
    }
}

void string_clear(String *string) {
    string_sanitize(string);

    if (string_on_stack(string)) {
        string->capacity = 0;
    } else {
        string->heap_buffer.size = 0;
    }

    string_add_nullbyte(string);
}

void string_insert(String *string, usize index, u8 c) {
    string_sanitize(string);
    assert(index <= string_size(string));
    string_resize_if_needed(string, 1);
    memmove(
        string_data(string) + index + 1,
        string_data(string) + index,
        string_size(string) - index
    );
    string_data(string)[index] = c;
    string_increase_size(string, 1);
    string_add_nullbyte(string);
}

void string_insert_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
) {
    string_sanitize(string);
    assert(index <= string_size(string));
    string_resize_if_needed(string, size);
    memmove(
        string_data(string) + index + size,
        string_data(string) + index,
        string_size(string) - index
    );
    memcpy(string_data(string) + index, data, size);
    string_increase_size(string, size);
    string_add_nullbyte(string);
}

void string_insert_strview(String *string, usize index, StringView other) {
    string_sanitize(string);
    assert(index <= string_size(string));
    string_insert_range(string, index, other.data, other.size);
}

void string_push_back(String *string, u8 c) {
    string_sanitize(string);
    string_resize_if_needed(string, 1);
    string_data(string)[string_size(string)] = c;
    string_increase_size(string, 1);
    string_add_nullbyte(string);
}

void string_push_back_range(String *restrict string, const u8 *restrict data, usize size) {
    string_sanitize(string);
    string_resize_if_needed(string, size);
    memcpy(string_data(string) + string_size(string), data, size);
    string_increase_size(string, size);
    string_add_nullbyte(string);
}

void string_push_back_strview(String *string, StringView other) {
    string_sanitize(string);
    string_push_back_range(string, other.data, other.size);
}

void string_push_back_i64(String *string, i64 value) {
    string_sanitize(string);

    if (value < 0) {
        string_push_back(string, '-');
        string_push_back_u64(string, -(u64)value);
    } else {
        string_push_back_u64(string, value);
    }
}

void string_push_back_u64(String *string, u64 value) {
    string_sanitize(string);

    u8 local_buf[20];
    usize i = 20;

    do {
        --i;
        local_buf[i] = (value % 10) + '0';
        value /= 10;
    } while (value != 0 && i != 0);

    string_push_back_range(string, &local_buf[i], 20 - i);
}

void string_erase(String *restrict string, usize index) {
    string_sanitize(string);
    assert(index < string_size(string));
    string_decrease_size(string, 1);
    memmove(
        string_data(string) + index,
        string_data(string) + index + 1,
        string_size(string) - index
    );
    string_add_nullbyte(string);
}

void string_erase_range(String *restrict string, usize start, usize end) {
    string_sanitize(string);
    assert(start <= string_size(string));
    assert(end <= string_size(string));
    string_decrease_size(string, end - start);
    memmove(string_data(string) + start, string_data(string) + end, string_size(string) - start);
    string_add_nullbyte(string);
}

void string_pop_back(String *restrict string) {
    string_sanitize(string);
    string_decrease_size(string, 1);
    string_add_nullbyte(string);
}

void string_pop_back_range(String *restrict string, usize start) {
    string_sanitize(string);
    assert(start <= string_size(string));

    if (string_on_stack(string)) {
        string->capacity = start;
    } else {
        string->heap_buffer.size = start;
    }

    string_add_nullbyte(string);
}

void string_replace(String *restrict string, usize index, u8 c) {
    string_sanitize(string);
    assert(index < string_size(string));
    string_data(string)[index] = c;
}

void string_replace_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
) {
    string_sanitize(string);
    assert(index < string_size(string));
    assert(index + size <= string_size(string));
    memcpy(string_data(string) + index, data, size);
}

void string_replace_strview(String *string, usize index, StringView other) {
    string_sanitize(string);
    string_replace_range(string, index, other.data, other.size);
}
