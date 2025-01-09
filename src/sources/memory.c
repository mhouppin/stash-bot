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

#include "memory.h"

bool mem_equal(const u8 *data, const u8 *other, usize size) {
    for (usize i = 0; i < size; ++i) {
        if (data[i] != other[i]) {
            return false;
        }
    }

    return true;
}

i32 mem_compare(const u8 *data, const u8 *other, usize size) {
    for (usize i = 0; i < size; ++i) {
        if (data[i] != other[i]) {
            return (i32)data[i] - (i32)other[i];
        }
    }

    return 0;
}

static u8 ascii_tolower(u8 b) {
    return (b >= 'A' && b <= 'Z') ? b - 'A' + 'a' : b;
}

i32 mem_compare_nocase(const u8 *data, const u8 *other, usize size) {
    for (usize i = 0; i < size; ++i) {
        u8 b1 = ascii_tolower(data[i]);
        u8 b2 = ascii_tolower(other[i]);

        if (b1 != b2) {
            return (i32)b1 - (i32)b2;
        }
    }

    return 0;
}

usize mem_byte_index(const u8 *data, u8 c, usize size) {
    for (usize i = 0; i < size; ++i) {
        if (data[i] == c) {
            return i;
        }
    }

    return NPOS;
}

usize mem_segm_index(const u8 *data, usize size, const u8 *segm, usize segmsize) {
    usize i = 0;

    if (segmsize == 0) {
        return 0;
    }

    while (i + segmsize <= size) {
        usize next = mem_byte_index(data + i, *segm, size - i - segmsize + 1);

        if (next == NPOS) {
            break;
        }

        if (mem_equal(data + next, segm, segmsize)) {
            return next;
        }

        i = next + 1;
    }

    return NPOS;
}

usize mem_byte_rindex(const u8 *data, u8 c, usize size) {
    for (usize i = size; i > 0; --i) {
        if (data[i - 1] == c) {
            return i - 1;
        }
    }

    return NPOS;
}

usize mem_segm_rindex(const u8 *data, usize size, const u8 *segm, usize segmsize) {
    usize i = size - segmsize + 1;

    if (segmsize == 0) {
        return size;
    }

    if (size < segmsize) {
        return NPOS;
    }

    while (i > 0) {
        usize next = mem_byte_rindex(data, *segm, i);

        if (next == NPOS) {
            break;
        }

        if (mem_equal(data + next, segm, segmsize)) {
            return next;
        }

        i = next;
    }

    return NPOS;
}

usize mem_byte_span(const u8 *data, usize size, const u8 *accept, usize accept_size) {
    u8 table[256] = {0};
    usize i;

    for (i = 0; i < accept_size; ++i) {
        table[accept[i]] = 1;
    }

    for (i = 0; i < size; ++i) {
        if (!table[data[i]]) {
            break;
        }
    }

    return i;
}

usize mem_byte_rspan(const u8 *data, usize size, const u8 *accept, usize accept_size) {
    u8 table[256] = {0};
    usize i;

    for (i = 0; i < accept_size; ++i) {
        table[accept[i]] = 1;
    }

    for (i = size; i > 0; --i) {
        if (!table[data[i - 1]]) {
            break;
        }
    }

    return size - i;
}

usize mem_byte_nspan(const u8 *data, usize size, const u8 *reject, usize reject_size) {
    u8 table[256] = {0};
    usize i;

    for (i = 0; i < reject_size; ++i) {
        table[reject[i]] = 1;
    }

    for (i = 0; i < size; ++i) {
        if (table[data[i]]) {
            break;
        }
    }

    return i;
}

usize mem_byte_rnspan(const u8 *data, usize size, const u8 *reject, usize reject_size) {
    u8 table[256] = {0};
    usize i;

    for (i = 0; i < reject_size; ++i) {
        table[reject[i]] = 1;
    }

    for (i = size; i > 0; --i) {
        if (table[data[i - 1]]) {
            break;
        }
    }

    return size - i;
}
