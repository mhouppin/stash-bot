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

#ifndef SYNCIO_H
#define SYNCIO_H

#include <stdio.h>

#include "strmanip.h"
#include "strview.h"

void sync_init(void);

void sync_lock_stdout(void);

void sync_unlock_stdout(void);

usize string_getline(FILE *f, String *string);

usize fwrite_strview(FILE *f, StringView strview);

INLINED usize fwrite_string(FILE *f, const String *string) {
    return fwrite_strview(f, strview_from_string(string));
}

#endif
