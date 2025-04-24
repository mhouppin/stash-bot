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

#ifndef WMALLOC_H
#define WMALLOC_H

#include <assert.h>

#include "core.h"

#define wrap_malloc(size) wrap_malloc_internal(size, __BASE_FILE__, __LINE__)

#define wrap_realloc(oldptr, new_size) \
    wrap_realloc_internal(oldptr, new_size, __BASE_FILE__, __LINE__)

#define wrap_aligned_alloc(alignment, size) \
    wrap_aligned_alloc_internal(alignment, size, __BASE_FILE__, __LINE__)

#define wrap_aligned_free(ptr) wrap_aligned_free_internal(ptr, __BASE_FILE__, __LINE__)

// Wrapper function for malloc() that will abort execution if the allocation fails. Don't use this
// API section directly, use wrap_malloc() instead.
void *wrap_malloc_internal(usize size, const char *source_file, usize line);

// Wrapper function for realloc() that will abort execution if the allocation fails. Don't use this
// API section directly, use wrap_realloc() instead.
void *wrap_realloc_internal(void *oldptr, usize new_size, const char *source_file, usize line);

// Wrapper function for alignment-constrained allocations that will abort execution if the
// allocation fails. Don't use this API section directly, use wrap_aligned_alloc() instead.
void *wrap_aligned_alloc_internal(usize alignment, usize size, const char *source_file, usize line);

// Wrapper function for alignment-constrained memory frees. Don't use this API section directly, use
// wrap_aligned_free() instead.
void wrap_aligned_free_internal(void *ptr, const char *source_file, usize line);

#endif
