/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#include "endgame.h"
#include "movelist.h"
#include "option.h"
#include "search.h"
#include "timeman.h"
#include "tt.h"
#include "tuner.h"
#include "uci.h"
#include "worker.h"
#include <pthread.h>
#include <stdio.h>

board_t Board;
pthread_attr_t WorkerSettings;
goparams_t SearchParams;
option_list_t OptionList;
movelist_t SearchMoves;

uint64_t Seed = 1048592ul;

ucioptions_t Options = {1, 16, 100, 1, false, false};

timeman_t Timeman;

const char *Delimiters = " \r\t\n";

int main(int argc, char **argv)
{
    bitboard_init();
    psq_score_init();
    zobrist_init();
    cyclic_init();
    init_kpk_bitbase();
    init_endgame_table();

#ifdef TUNE

    if (argc != 2)
    {
        printf("Usage: %s dataset_file\n", *argv);
        return (0);
    }
    start_tuning_session(argv[1]);

#else

    tt_resize(16);
    init_search_tables();
    pthread_attr_init(&WorkerSettings);
    pthread_attr_setstacksize(&WorkerSettings, 4ul * 1024 * 1024);
    wpool_init(&WPool, 1);

    // Wait for the engine thread to be ready

    worker_wait_search_end(wpool_main_worker(&WPool));
    uci_loop(argc, argv);
    wpool_init(&WPool, 0);

#endif

    return (0);
}
