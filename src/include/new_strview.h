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

#ifndef STRVIEW_H
#define STRVIEW_H

#include "new_core.h"
#include "new_memory.h"

#define STATIC_STRVIEW(s) \
    (StringView) { \
        .data = (const u8 *)(s), .size = sizeof(s) - 1 \
    }

INLINED usize cstr_length(const char *cstr) {
    usize i = 0;
    while (cstr[i] != '\0')
        ++i;
    return i;
}

typedef struct _StringView {
    const u8 *data;
    usize size;
} StringView;

static const StringView EmptyStrview = {.data = NULL, .size = 0};

extern const StringView Whitespaces;

INLINED StringView strview_from_raw_data(const u8 *data, usize size) {
    return (StringView) {data, size};
}

INLINED StringView strview_from_cstr(const char *data) {
    return (StringView) {(const u8 *)data, cstr_length(data)};
}

INLINED StringView strview_subview(StringView strview, usize start, usize end) {
    return (StringView) {strview.data + start, end - start};
}

usize strview_find(StringView strview, u8 c);
usize strview_find_range(StringView strview, const u8 *data, usize size);
usize strview_find_strview(StringView strview, StringView other);

usize strview_rfind(StringView strview, u8 c);
usize strview_rfind_range(StringView strview, const u8 *data, usize size);
usize strview_rfind_strview(StringView strview, StringView other);

usize strview_find_first_of_range(StringView strview, const u8 *data, usize size);
usize strview_find_first_of_strview(StringView strview, StringView other);

usize strview_find_first_not_of_range(StringView strview, const u8 *data, usize size);
usize strview_find_first_not_of_strview(StringView strview, StringView other);

usize strview_find_last_of_range(StringView strview, const u8 *data, usize size);
usize strview_find_last_of_strview(StringView strview, StringView other);

usize strview_find_last_not_of_range(StringView strview, const u8 *data, usize size);
usize strview_find_last_not_of_strview(StringView strview, StringView other);

i32 strview_compare_range(StringView strview, const u8 *data, usize size);
i32 strview_compare_strview(StringView strview, StringView other);

bool strview_starts_with(StringView strview, u8 c);
bool strview_starts_with_range(StringView strview, const u8 *data, usize size);
bool strview_starts_with_strview(StringView strview, StringView other);

bool strview_ends_with(StringView strview, u8 c);
bool strview_ends_with_range(StringView strview, const u8 *data, usize size);
bool strview_ends_with_strview(StringView strview, StringView other);

bool strview_equals(StringView strview, u8 c);
bool strview_equals_range(StringView strview, const u8 *data, usize size);
bool strview_equals_strview(StringView strview, StringView other);

bool strview_equals_range_nocase(StringView strview, const u8 *data, usize size);
bool strview_equals_strview_nocase(StringView strview, StringView other);

StringView strview_trim_whitespaces(StringView strview);
StringView strview_next_word(StringView *strview);

bool strview_parse_i64(StringView strview, i64 *value);
bool strview_parse_u64(StringView strview, u64 *value);

#endif
