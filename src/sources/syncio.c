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

#include "syncio.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    MAX_UCI_LINE_LENGTH = 16384,
};

static pthread_mutex_t StdoutMutex;
static atomic_bool UciDebug;

void sync_init(void) {
    if (pthread_mutex_init(&StdoutMutex, NULL)) {
        perror("Unable to initialize stdout mutex");
        exit(EXIT_FAILURE);
    }

    atomic_init(&UciDebug, false);
}

void sync_lock_stdout(void) {
    pthread_mutex_lock(&StdoutMutex);
}

void sync_unlock_stdout(void) {
    pthread_mutex_unlock(&StdoutMutex);
}

usize fwrite_strview(FILE *f, StringView strview) {
    return fwrite(strview.data, sizeof(u8), strview.size, f);
}

void toggle_debug(bool state) {
    atomic_store_explicit(&UciDebug, state, memory_order_relaxed);
}

void info_debug(const char *fmt, ...) {
    if (!atomic_load_explicit(&UciDebug, memory_order_relaxed)) {
        return;
    }

    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
}

usize string_getline(FILE *f, String *string) {
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_UCI_LINE_LENGTH] = {0};

    string_clear(string);

    if (fgets(buffer, sizeof(buffer), f) == NULL) {
        return 0;
    }

    if (string->capacity < (usize)MAX_UCI_LINE_LENGTH) {
        string_reserve(string, (usize)MAX_UCI_LINE_LENGTH);
    }

    // Note that this doesn't work if we get nullbytes in the middle of the string, but we shouldn't
    // be sent binary data over stdin anyway.
    usize nullbyte = mem_byte_index((u8 *)buffer, '\0', MAX_UCI_LINE_LENGTH);

    string_push_back_range(string, (u8 *)buffer, nullbyte);

    return nullbyte;
#else
    char *ptr = (char *)string->data;
    size_t size = (size_t)string->capacity;
    isize result = getline(&ptr, &size, f);

    if (result < 0) {
        string->size = 0;
        return 0;
    }

    string->data = (u8 *)ptr;
    string->size = (usize)result;
    string->capacity = (usize)size;
    return (usize)result;
#endif
}
