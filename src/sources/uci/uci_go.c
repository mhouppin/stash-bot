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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "info.h"
#include "movelist.h"
#include "uci.h"

void    uci_go(const char *args)
{
    wait_search_end();
    pthread_mutex_lock(&EngineMutex);

    EngineSend = DO_THINK;
    memset(&SearchParams, 0, sizeof(goparams_t));
    list_all(&SearchMoves, &Board);

    char    *copy = strdup(args ? args : "");
    char    *token = strtok(copy, Delimiters);

    while (token)
    {
        if (strcmp(token, "searchmoves") == 0)
        {
            token = strtok(NULL, Delimiters);
            extmove_t   *m = SearchMoves.moves;
            while (token)
            {
                (m++)->move = str_to_move(&Board, token);
                token = strtok(NULL, Delimiters);
            }
            SearchMoves.last = m;
            break ;
        }
        else if (strcmp(token, "wtime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.wtime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "btime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.btime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "winc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.winc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "binc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.binc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "movestogo") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.movestogo = atoi(token);
        }
        else if (strcmp(token, "depth") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.depth = atoi(token);
        }
        else if (strcmp(token, "nodes") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.nodes = (size_t)atoll(token);
        }
        else if (strcmp(token, "mate") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.mate = atoi(token);
        }
        else if (strcmp(token, "perft") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.perft = atoi(token);
        }
        else if (strcmp(token, "movetime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.movetime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "infinite") == 0)
            SearchParams.infinite = 1;

        token = strtok(NULL, Delimiters);
    }

    EngineMode = THINKING;

    pthread_cond_broadcast(&EngineCond);
    pthread_mutex_unlock(&EngineMutex);
    free(copy);
}
