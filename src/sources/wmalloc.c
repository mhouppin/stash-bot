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

#include "wmalloc.h"

#include <stdlib.h>
#include <unistd.h>

#include "strview.h"

// This is required because Microsoft's C runtime library doesn't implement aligned_alloc().
#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

static isize log_usize(usize value) {
    char buf[20];
    usize i = 20;

    do {
        --i;
        buf[i] = (char)(value % 10) + '0';
        value /= 10;
    } while (i != 0 && value != 0);

    return write(STDERR_FILENO, &buf[i], 20 - i);
}

static isize log_string(const char *cstr) {
    const usize len = cstr_length(cstr);

    return write(STDERR_FILENO, cstr, len);
}

void *wrap_malloc_internal(usize size, const char *source_file, usize line) {
    void *ptr = malloc(size);

    if (ptr == NULL && size != 0) {
        log_string(source_file);
        log_string(":");
        log_usize(line);
        log_string(": allocation of ");
        log_usize(size);
        log_string(" bytes failed. Aborting.");
        exit(12);
    }

    return ptr;
}

void *wrap_realloc_internal(void *oldptr, usize new_size, const char *source_file, usize line) {
    void *ptr = realloc(oldptr, new_size);

    if (ptr == NULL && new_size != 0) {
        log_string(source_file);
        log_string(":");
        log_usize(line);
        log_string(": reallocation of ");
        log_usize(new_size);
        log_string(" bytes failed. Aborting.");
        exit(12);
    }

    return ptr;
}

void *
    wrap_aligned_alloc_internal(usize alignment, usize size, const char *source_file, usize line) {

#if defined(_WIN32) || defined(_WIN64)
    void *ptr = _aligned_malloc(size, alignment);
#else
    void *ptr = aligned_alloc(alignment, size);
#endif

    if (ptr == NULL && size != 0) {
        log_string(source_file);
        log_string(":");
        log_usize(line);
        log_string(": allocation of ");
        log_usize(size);
        log_string(" bytes failed. Aborting.");
        exit(12);
    }

    return ptr;
}

void wrap_aligned_free_internal(
    void *ptr,
    __attribute__((unused)) const char *source_file,
    __attribute__((unused)) usize line
) {
#if defined(_WIN32) || defined(_WIN64)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}
