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

#ifndef STRMANIP_H
#define STRMANIP_H

#include "new_core.h"
#include "new_strview.h"

static const usize STRING_MIN_ALLOC = 16;

typedef struct _String {
    u8 *data;
    usize size;
    usize capacity;
} String;

INLINED StringView strview_from_string(const String *string) {
    return (StringView) {string->data, string->size};
}

void string_init(String *string);
void string_init_copy(String *restrict string, const u8 *restrict data, usize size);
void string_init_borrow(String *string, u8 *data, usize size);
void string_init_from_cstr(String *restrict string, const char *data);
void string_init_from_strview(String *string, StringView other);

void string_destroy(String *string);

void string_reserve(String *string, usize new_capacity);
void string_auto_resize(String *string, usize min_resize);
void string_shrink_to_fit(String *string);

void string_clear(String *string);

void string_insert(String *string, usize index, u8 c);
void string_insert_range(String *restrict string, usize index, const u8 *restrict data, usize size);
void string_insert_strview(String *string, usize index, StringView other);

void string_push_back(String *string, u8 c);
void string_push_back_range(String *restrict string, const u8 *restrict data, usize size);
void string_push_back_strview(String *string, StringView other);
void string_push_back_i64(String *string, i64 value);
void string_push_back_u64(String *string, u64 value);

void string_erase(String *restrict string, usize index);
void string_erase_range(String *restrict string, usize start, usize end);

void string_pop_back(String *restrict string);
void string_pop_back_range(String *restrict string, usize start);

void string_replace(String *restrict string, usize index, u8 c);
void string_replace_range(
    String *restrict string,
    usize index,
    const u8 *restrict data,
    usize size
);
void string_replace_strview(String *string, usize index, StringView other);

#endif
