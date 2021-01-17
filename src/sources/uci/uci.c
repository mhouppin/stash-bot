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

#include "info.h"
#include "lazy_smp.h"
#include "option.h"
#include "tt.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const cmdlink_t commands[] =
{
    {"bench", &uci_bench},
    {"d", &uci_d},
    {"go", &uci_go},
    {"isready", &uci_isready},
    {"position", &uci_position},
    {"quit", &uci_quit},
    {"setoption", &uci_setoption},
    {"stop", &uci_stop},
    {"uci", &uci_uci},
    {"ucinewgame", &uci_ucinewgame},
    {NULL, NULL}
};

int execute_uci_cmd(const char *command)
{
    char    *dup = strdup(command);
    char    *cmd = strtok(dup, Delimiters);

    if (!cmd)
    {
        free(dup);
        return (1);
    }

    for (size_t i = 0; commands[i].cmd_name != NULL; ++i)
    {
        if (strcmp(commands[i].cmd_name, cmd) == 0)
        {
            commands[i].call(strtok(NULL, ""));
            break ;
        }
    }

    if (strcmp(cmd, "quit") == 0)
    {
        free(dup);
        return (0);
    }

    free(dup);
    return (1);
}

void    on_hash_set(void *data)
{
    tt_resize((size_t)*(long *)data);
    printf("info string set Hash to %lu MB\n", *(long *)data);
    fflush(stdout);
}

void    on_clear_hash(void *nothing __attribute__((unused)))
{
    tt_bzero();
    puts("info string cleared hash");
    fflush(stdout);
}

void    on_thread_set(void *data)
{
    wpool_init((int)*(long *)data);
    printf("info string set Threads to %lu\n", *(long *)data);
    fflush(stdout);
}

void    uci_loop(int argc, char **argv)
{
    init_option_list(&OptionList);
    add_option_spin_int(&OptionList, "Threads", &Options.threads, 1, 256, &on_thread_set);
    add_option_spin_int(&OptionList, "Hash", &Options.hash, 1, MAX_HASH, &on_hash_set);
    add_option_spin_int(&OptionList, "Move Overhead", &Options.move_overhead, 0, 30000, NULL);
    add_option_spin_int(&OptionList, "MultiPV", &Options.multi_pv, 1, 500, NULL);
    add_option_check(&OptionList, "UCI_Chess960", &Options.chess960, NULL);
    add_option_button(&OptionList, "Clear Hash", &on_clear_hash);

    uci_position("startpos");

    if (argc > 1)
        for (int i = 1; i < argc; ++i)
            execute_uci_cmd(argv[i]);
    else
    {
        char    *line = malloc(16384);

        while (fgets(line, 16384, stdin) != NULL)
            if (execute_uci_cmd(line) == 0)
                break ;

        free(line);
    }

    wait_search_end();
    uci_quit(NULL);
    quit_option_list(&OptionList);
}

