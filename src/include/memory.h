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

#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

static const usize NPOS = SIZE_MAX;

bool mem_equal(const u8 *data, const u8 *other, usize size);
i32 mem_compare(const u8 *data, const u8 *other, usize size);
i32 mem_compare_nocase(const u8 *data, const u8 *other, usize size);

usize mem_byte_index(const u8 *data, u8 c, usize size);
usize mem_segm_index(const u8 *data, usize size, const u8 *segm, usize segmsize);
usize mem_byte_rindex(const u8 *data, u8 c, usize size);
usize mem_segm_rindex(const u8 *data, usize size, const u8 *segm, usize segmsize);

usize mem_byte_span(const u8 *data, usize size, const u8 *accept, usize accept_size);
usize mem_byte_rspan(const u8 *data, usize size, const u8 *accept, usize accept_size);
usize mem_byte_nspan(const u8 *data, usize size, const u8 *reject, usize reject_size);
usize mem_byte_rnspan(const u8 *data, usize size, const u8 *reject, usize reject_size);

#endif
