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

#include "strmanip.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "strview.h"
#include "wmalloc.h"

void string_init(String *string) {
    string->data = NULL;
    string->size = 0;
    string->capacity = 0;
}

void string_init_copy(String *restrict string, const u8 *restrict data, usize size) {
    string->capacity = usize_max(size, STRING_MIN_ALLOC);
    string->data = wrap_malloc(string->capacity);
    string->size = size;
    memcpy(string->data, data, size);
}

void string_init_borrow(String *string, u8 *data, usize size) {
    string->data = data;
    string->capacity = size;
    string->size = size;
}

void string_init_from_cstr(String *restrict string, const char *data) {
    string_init_copy(string, (const u8 *)data, strlen(data));
}

void string_init_from_strview(String *string, StringView other) {
    string_init_copy(string, other.data, other.size);
}

void string_destroy(String *string) {
    free(string->data);
    string_init(string);
}

void string_reserve(String *string, usize new_capacity) {
    new_capacity = usize_max(new_capacity, string->size);
    new_capacity = usize_max(new_capacity, STRING_MIN_ALLOC);
    string->data = wrap_realloc(string->data, new_capacity);
    string->capacity = new_capacity;
}

void string_auto_resize(String *string, usize min_resize) {
    usize new_capacity = usize_max(string->capacity * 3 / 2, string->capacity + min_resize);
    string_reserve(string, new_capacity);
}

void string_shrink_to_fit(String *string) {
    if (string->size == 0) {
        free(string->data);
        string->data = NULL;
        string->capacity = 0;
    } else {
        string_reserve(string, string->size);
    }
}

void string_clear(String *string) {
    string->size = 0;
}

void string_insert(String *string, usize index, u8 c) {
    if (string->size == string->capacity) {
        string_auto_resize(string, 1);
    }

    memmove(string->data + index + 1, string->data + index, string->size - index);
    string->data[index] = c;
    ++string->size;
}

void string_insert_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
) {
    if (string->size + size > string->capacity) {
        string_auto_resize(string, string->size + size - string->capacity);
    }

    memmove(string->data + index + size, string->data + index, string->size - index);
    memcpy(string->data + index, data, size);
    string->size += size;
}

void string_insert_strview(String *string, usize index, StringView other) {
    string_insert_range(string, index, other.data, other.size);
}

void string_push_back(String *string, u8 c) {
    if (string->size == string->capacity) {
        string_auto_resize(string, 1);
    }

    string->data[string->size] = c;
    ++string->size;
}

void string_push_back_range(String *restrict string, const u8 *restrict data, usize size) {
    if (string->size + size > string->capacity) {
        string_auto_resize(string, string->size + size - string->capacity);
    }

    memcpy(string->data + string->size, data, size);
    string->size += size;
}

void string_push_back_strview(String *string, StringView other) {
    string_push_back_range(string, other.data, other.size);
}

void string_push_back_i64(String *string, i64 value) {
    if (value < 0) {
        string_push_back(string, '-');
        string_push_back_u64(string, -(u64)value);
    } else {
        string_push_back_u64(string, value);
    }
}

void string_push_back_u64(String *string, u64 value) {
    u8 local_buf[20];
    usize i = 20;

    do {
        --i;
        local_buf[i] = (value % 10) + '0';
        value /= 10;
    } while (value != 0 && i != 0);

    string_push_back_strview(string, strview_from_raw_data(&local_buf[i], 20 - i));
}

void string_erase(String *restrict string, usize index) {
    --string->size;
    memmove(string->data + index, string->data + index + 1, string->size - index);
}

void string_erase_range(String *restrict string, usize start, usize end) {
    string->size -= end - start;
    memmove(string->data + start, string->data + end, string->size - start);
}

void string_pop_back(String *restrict string) {
    --string->size;
}

void string_pop_back_range(String *restrict string, usize start) {
    string->size = start;
}

void string_replace(String *restrict string, usize index, u8 c) {
    string->data[index] = c;
}

void string_replace_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
) {
    memcpy(string->data + index, data, size);
}

void string_replace_strview(String *string, usize index, StringView other) {
    string_replace_range(string, index, other.data, other.size);
}
