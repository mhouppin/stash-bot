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

#include "endgame.h"
#include "engine.h"
#include "lazy_smp.h"
#include "option.h"
#include "timeman.h"
#include "tt.h"
#include "tuner.h"
#include "uci.h"
#include <pthread.h>
#include <stdio.h>

board_t Board;
pthread_attr_t WorkerSettings;
goparams_t SearchParams;
option_list_t OptionList;
movelist_t SearchMoves;

pthread_cond_t EngineCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t EngineMutex = PTHREAD_MUTEX_INITIALIZER;
enum e_egn_mode EngineMode = THINKING;
enum e_egn_send EngineSend = DO_NOTHING;
int EnginePonderhit = 0;

uint64_t Seed = 1048592ul;

ucioptions_t Options = {
    1, 16, 100, 1, false
};

timeman_t Timeman;

const char *Delimiters = " \t\n";

int main(int argc, char **argv)
{
    bitboard_init();
    psq_score_init();
    zobrist_init();
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
    wpool_init(1);
    init_reduction_table();

    // Start the main engine thread

    pthread_t engineThread;

    pthread_attr_init(&WorkerSettings);
    pthread_attr_setstacksize(&WorkerSettings, 4ul * 1024 * 1024);

    if (pthread_create(&engineThread, &WorkerSettings, &engine_thread, NULL))
    {
        perror("Failed to boot engine thread");
        return (1);
    }

    // Wait for the engine thread to be ready

    wait_search_end();

    uci_loop(argc, argv);

    wpool_quit();

#endif

    return (0);
}
