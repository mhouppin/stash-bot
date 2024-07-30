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

#ifndef UCI_H
#define UCI_H

#include "new_board.h"
#include "new_core.h"
#include "new_option.h"
#include "new_strview.h"
#include "new_worker.h"

typedef struct _OptionValues {
    i64 threads;
    i64 hash;
    i64 move_overhead;
    i64 multi_pv;
    bool chess960;
    bool ponder;
    bool show_wdl;
    bool normalize_score;
} OptionValues;

typedef struct _Uci {
    OptionValues option_values;
    OptionList option_list;
    Board root_board;
    WorkerPool worker_pool;
    atomic_bool debug_mode;
} Uci;

typedef struct _Command {
    StringView cmd_name;
    void (*cmd_exec)(Uci *, StringView);
} Command;

void uci_init(Uci *uci);
void uci_destroy(Uci *uci);

// The list of supported commands by the engine
void uci_bench(Uci *uci, StringView args);
void uci_d(Uci *uci, StringView args);
void uci_debug(Uci *uci, StringView args);
void uci_go(Uci *uci, StringView args);
void uci_isready(Uci *uci, StringView args);
void uci_ponderhit(Uci *uci, StringView args);
void uci_position(Uci *uci, StringView args);
void uci_quit(Uci *uci, StringView args);
void uci_setoption(Uci *uci, StringView args);
void uci_stop(Uci *uci, StringView args);
void uci_t(Uci *uci, StringView args);
void uci_uci(Uci *uci, StringView args);
void uci_ucinewgame(Uci *uci, StringView args);

// A nice API entry point to directly execute some commands, that returns true if the UCI thread
// should keep parsing commands
bool uci_exec_command(Uci *uci, StringView command);

// Main entry point for the UCI handler
void uci_loop(int argc, char **argv);

#endif
