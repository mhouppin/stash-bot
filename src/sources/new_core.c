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

#include "new_core.h"

#if defined(_WIN32) || defined(_WIN64)
#include <sys/timeb.h>
#else
#include <time.h>
#endif

Timepoint timepoint_now(void) {
#if defined(_WIN32) || defined(_WIN64)
    struct timeb tp;

    ftime(&tp);
    return (Timepoint)tp.time * 1000 + (Timepoint)tp.millitm;
#else
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    return (Timepoint)tp.tv_sec * 1000 + (Timepoint)tp.tv_nsec / 1000000;
#endif
}
