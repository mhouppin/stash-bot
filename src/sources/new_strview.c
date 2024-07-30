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

#include "new_strview.h"

#include "new_core.h"
#include "new_memory.h"

const StringView Whitespaces = {
    .data = (const u8 *)" \t\n\r",
    .size = 4,
};

usize strview_find(StringView strview, u8 c) {
    return mem_byte_index(strview.data, c, strview.size);
}

usize strview_find_range(StringView strview, const u8 *data, usize size) {
    return mem_segm_index(strview.data, strview.size, data, size);
}

usize strview_find_strview(StringView strview, StringView other) {
    return strview_find_range(strview, other.data, other.size);
}

usize strview_rfind(StringView strview, u8 c) {
    return mem_byte_rindex(strview.data, c, strview.size);
}

usize strview_rfind_range(StringView strview, const u8 *data, usize size) {
    return mem_segm_rindex(strview.data, strview.size, data, size);
}

usize strview_rfind_strview(StringView strview, StringView other) {
    return strview_rfind_range(strview, other.data, other.size);
}

usize strview_find_first_of_range(StringView strview, const u8 *data, usize size) {
    usize span = mem_byte_nspan(strview.data, strview.size, data, size);
    return span < strview.size ? span : NPOS;
}

usize strview_find_first_of_strview(StringView strview, StringView other) {
    return strview_find_first_of_range(strview, other.data, other.size);
}

usize strview_find_first_not_of_range(StringView strview, const u8 *data, usize size) {
    usize span = mem_byte_span(strview.data, strview.size, data, size);
    return span < strview.size ? span : NPOS;
}

usize strview_find_first_not_of_strview(StringView strview, StringView other) {
    return strview_find_first_not_of_range(strview, other.data, other.size);
}

usize strview_find_last_of_range(StringView strview, const u8 *data, usize size) {
    usize span = mem_byte_rnspan(strview.data, strview.size, data, size);
    return span < strview.size ? strview.size - span - 1 : NPOS;
}

usize strview_find_last_of_strview(StringView strview, StringView other) {
    return strview_find_last_of_range(strview, other.data, other.size);
}

usize strview_find_last_not_of_range(StringView strview, const u8 *data, usize size) {
    usize span = mem_byte_rspan(strview.data, strview.size, data, size);
    return span < strview.size ? strview.size - span - 1 : NPOS;
}

usize strview_find_last_not_of_strview(StringView strview, StringView other) {
    return strview_find_last_not_of_range(strview, other.data, other.size);
}

i32 strview_compare_range(StringView strview, const u8 *data, usize size) {
    usize minsize = usize_min(strview.size, size);
    i32 result = mem_compare(strview.data, data, minsize);

    if (result != 0) {
        return result;
    }

    return strview.size > minsize ? 1 : strview.size < minsize ? -1 : 0;
}

i32 strview_compare_strview(StringView strview, StringView other) {
    return strview_compare_range(strview, other.data, other.size);
}

bool strview_starts_with(StringView strview, u8 c) {
    return strview.size > 0 && strview.data[0] == c;
}

bool strview_starts_with_range(StringView strview, const u8 *data, usize size) {
    return strview.size >= size && mem_equal(strview.data, data, size);
}

bool strview_starts_with_strview(StringView strview, StringView other) {
    return strview_starts_with_range(strview, other.data, other.size);
}

bool strview_ends_with(StringView strview, u8 c) {
    return strview.size > 0 && strview.data[strview.size - 1] == c;
}

bool strview_ends_with_range(StringView strview, const u8 *data, usize size) {
    return strview.size >= size && mem_equal(strview.data + strview.size - size, data, size);
}

bool strview_ends_with_strview(StringView strview, StringView other) {
    return strview_ends_with_range(strview, other.data, other.size);
}

bool strview_equals(StringView strview, u8 c) {
    return strview.size == 1 && strview.data[0] == c;
}

bool strview_equals_range(StringView strview, const u8 *data, usize size) {
    return strview.size == size && mem_equal(strview.data, data, size);
}

bool strview_equals_strview(StringView strview, StringView other) {
    return strview_equals_range(strview, other.data, other.size);
}

bool strview_equals_range_nocase(StringView strview, const u8 *data, usize size) {
    return strview.size == size && !mem_compare_nocase(strview.data, data, size);
}

bool strview_equals_strview_nocase(StringView strview, StringView other) {
    return strview_equals_range_nocase(strview, other.data, other.size);
}

StringView strview_trim_whitespaces(StringView strview) {
    StringView wspace = STATIC_STRVIEW(" \t\n\r");
    usize first, last;

    first = strview_find_first_not_of_strview(strview, wspace);

    if (first == NPOS) {
        return strview_subview(strview, strview.size, strview.size);
    }

    last = strview_find_last_not_of_strview(strview, wspace);

    return strview_subview(strview, first, last + 1);
}

StringView strview_next_word(StringView *strview) {
    StringView wspace = STATIC_STRVIEW(" \t\n\r");
    StringView ltrimmed;
    usize word_begin, word_size;

    word_begin = strview_find_first_not_of_strview(*strview, wspace);

    if (word_begin == NPOS) {
        *strview = strview_subview(*strview, strview->size, strview->size);
        return *strview;
    }

    ltrimmed = strview_subview(*strview, word_begin, strview->size);
    word_size = strview_find_first_of_strview(ltrimmed, wspace);

    if (word_size == NPOS) {
        word_size = ltrimmed.size;
    }

    ltrimmed = strview_subview(ltrimmed, 0, word_size);
    *strview = strview_subview(*strview, word_begin + word_size, strview->size);

    return ltrimmed;
}

bool strview_parse_i64(StringView strview, i64 *value) {
    usize i = 0;
    i64 result = 0;
    bool negative = false;

    if (strview.size != 0 && (strview.data[0] == '-' || strview.data[0] == '+')) {
        negative = (strview.data[0] == '-');
        ++i;
    }

    if (strview.size == i) {
        return false;
    }

    while (i < strview.size) {
        if (strview.data[i] < '0' || strview.data[i] > '9') {
            return false;
        }

        result *= 10;
        result += strview.data[i] - '0';
        ++i;
    }

    *value = negative ? -result : result;
    return true;
}

bool strview_parse_u64(StringView strview, u64 *value) {
    usize i = 0;
    u64 result = 0;

    if (strview.size == 0) {
        return false;
    }

    while (i < strview.size) {
        if (strview.data[i] < '0' || strview.data[i] > '9') {
            return false;
        }

        result *= 10;
        result += strview.data[i] - '0';
        ++i;
    }

    *value = result;
    return true;
}
