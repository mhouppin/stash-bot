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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "new_bitboard.h"
#include "new_board.h"
#include "new_endgame.h"
#include "new_hashkey.h"
#include "new_kpk_bitbase.h"
#include "new_psq_table.h"
#include "new_search.h"
#include "new_syncio.h"
#include "new_tuner.h"
#include "new_uci.h"

int main(int argc, char **argv) {
    sync_init();
    bitboard_init();
    zobrist_init();
    psq_table_init();
    kpk_bitbase_init();
    endgame_table_init();
    cyclic_init();
    search_init();

#ifndef TUNE
    uci_loop(argc, argv);
#else
    if (argc == 1) {
        printf("usage: %s dataset_file_1 [dataset_file_2 ...]\n", *argv);
        return 1;
    }

    TunerConfig config;
    TunerDataset dataset;

    tuner_config_set_default_values(&config);
    tuner_dataset_init(&dataset);

    for (int i = 1; i < argc; ++i) {
        tuner_dataset_add_file(&dataset, argv[i]);
    }

    tuner_dataset_start_session(&dataset, &config);
    tuner_dataset_destroy(&dataset);
#endif
    return 0;
}
