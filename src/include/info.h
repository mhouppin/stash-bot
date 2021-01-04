/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

#ifndef INFO_H
# define INFO_H

# include <inttypes.h>
# include "board.h"
# include "move.h"
# include "movelist.h"

# ifdef PRIu64
#  define FMT_INFO  PRIu64
typedef uint64_t    info_t;
# define MAX_HASH   33554432
# else
#  define FMT_INFO  PRIu32
typedef uint32_t    info_t;
# define MAX_HASH   2048
# endif

const char  *move_to_str(move_t move, bool is_chess960);
const char  *score_to_str(score_t score);

move_t      str_to_move(const board_t *board, const char *str);

#endif
