/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#include "worker.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Small trick to detect if the system is 64-bit or 32-bit.
#if (SIZE_MAX == UINT64_MAX)
#define FMT_INFO PRIu64
#define KEY_INFO PRIx64
typedef uint64_t info_t;
#define MAX_HASH 33554432
#else
#define FMT_INFO PRIu32
#define KEY_INFO PRIx32
typedef uint32_t info_t;
#define MAX_HASH 2048
#endif

typedef struct _OptionFields
{
    long threads;
    long hash;
    long moveOverhead;
    long multiPv;
    long ratingAdv;
    score_t confidence;
    bool chess960;
    bool ponder;
    bool debug;
    bool showWDL;
    bool normalizeScore;
} OptionFields;

extern pthread_attr_t WorkerSettings;
extern OptionFields UciOptionFields;
extern const char *Delimiters;

typedef struct _CommandMap
{
    const char *commandName;
    void (*call)(const char *);
} CommandMap;

char *get_next_token(char **str);

const char *move_to_str(move_t move, bool isChess960);
const char *score_to_str(score_t score);
move_t str_to_move(const Board *board, const char *str);

// Displays the formatted content while in debug mode.
int debug_printf(const char *fmt, ...);

// The list of supported commands by the engine.
void uci_bench(const char *args);
void uci_d(const char *args);
void uci_debug(const char *args);
void uci_go(const char *args);
void uci_isready(const char *args);
void uci_ponderhit(const char *args);
void uci_position(const char *args);
void uci_quit(const char *args);
void uci_setoption(const char *args);
void uci_stop(const char *args);
void uci_uci(const char *args);
void uci_ucinewgame(const char *args);
void uci_loop(int argc, char **argv);

#endif
