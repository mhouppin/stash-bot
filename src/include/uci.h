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

#ifndef UCI_H
# define UCI_H

# include <inttypes.h>
# include <pthread.h>
# include <stdbool.h>
# include <stddef.h>
# include <time.h>
# include "lazy_smp.h"

# define UCI_VERSION "v31.18"

# if (SIZE_MAX == UINT64_MAX)
#  define FMT_INFO PRIu64
#  define KEY_INFO PRIx64
typedef uint64_t info_t;
# define MAX_HASH 33554432
# else
#  define FMT_INFO PRIu32
#  define KEY_INFO PRIx32
typedef uint32_t info_t;
# define MAX_HASH 2048
# endif

enum e_egn_mode
{
    WAITING,
    THINKING
};

enum e_egn_send
{
    DO_NOTHING,
    DO_THINK,
    DO_EXIT,
    DO_ABORT
};

typedef struct goparams_s
{
    clock_t wtime;
    clock_t btime;
    clock_t winc;
    clock_t binc;
    int movestogo;
    int depth;
    size_t nodes;
    int mate;
    int infinite;
    int perft;
    int ponder;
    clock_t movetime;
}
goparams_t;

typedef struct ucioptions_s
{
    long threads;
    long hash;
    long moveOverhead;
    long multiPv;
    bool chess960;
    bool ponder;
    bool showCurrLine;
}
ucioptions_t;

extern pthread_attr_t WorkerSettings;
extern ucioptions_t Options;
extern pthread_mutex_t EngineMutex;
extern pthread_cond_t EngineCond;
extern enum e_egn_mode EngineMode;
extern enum e_egn_send EngineSend;
extern int EnginePonderhit;
extern const char *Delimiters;
extern goparams_t SearchParams;

typedef struct cmdlink_s
{
    const char *commandName;
    void (*call)(const char *);
}
cmdlink_t;

INLINED bool search_should_abort(void)
{
    return (EngineSend == DO_EXIT || EngineSend == DO_ABORT);
}

void wait_search_end(void);
char *get_next_token(char **str);

const char *move_to_str(move_t move, bool isChess960);
const char *score_to_str(score_t score);
move_t str_to_move(const board_t *board, const char *str);
void print_pv(const board_t *board, root_move_t *rootMove, int multiPv,
    int depth, clock_t time, int bound);

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
