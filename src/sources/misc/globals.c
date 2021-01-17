/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#include <pthread.h>
#include "info.h"
#include "option.h"
#include "timeman.h"

board_t         Board;
pthread_attr_t  WorkerSettings;
goparams_t      SearchParams;
option_list_t   OptionList;
movelist_t      SearchMoves;

pthread_cond_t  EngineCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t EngineMutex = PTHREAD_MUTEX_INITIALIZER;
enum e_egn_mode EngineMode = THINKING;
enum e_egn_send EngineSend = DO_NOTHING;
uint64_t        Seed = 1048592ul;

ucioptions_t    Options = {
    1, 16, 100, 1, false
};

timeman_t       Timeman;

const char      *Delimiters = " \t\n";