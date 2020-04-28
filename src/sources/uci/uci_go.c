/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "info.h"
#include "movelist.h"
#include "uci.h"

void	uci_go(const char *args)
{
	const char	*delim = " \t\n";

	if (pthread_mutex_lock(&g_engine_mutex))
	{
		perror("Unable to launch engine");
		fflush(stderr);
		return ;
	}

	if (g_engine_mode == THINKING)
	{
		pthread_mutex_unlock(&g_engine_mutex);
		fputs("Engine already thinking", stderr);
		fflush(stderr);
		return ;
	}

	extern movelist_t	g_searchmoves;
	extern board_t		g_board;

	g_engine_send = DO_THINK;
	memset(&g_goparams, 0, sizeof(goparams_t));
	list_all(&g_searchmoves, &g_board);

	char	*copy = strdup(args ? args : "");
	char	*token = strtok(copy, delim);

	while (token)
	{
		if (strcmp(token, "searchmoves") == 0)
		{
			token = strtok(NULL, delim);
			extmove_t	*m = g_searchmoves.moves;
			while (token)
			{
				(m++)->move = str_to_move(&g_board, token);
				token = strtok(NULL, delim);
			}
			g_searchmoves.last = m;
			break ;
		}
		else if (strcmp(token, "wtime") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.wtime = (clock_t)atoll(token);
		}
		else if (strcmp(token, "btime") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.btime = (clock_t)atoll(token);
		}
		else if (strcmp(token, "winc") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.winc = (clock_t)atoll(token);
		}
		else if (strcmp(token, "binc") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.binc = (clock_t)atoll(token);
		}
		else if (strcmp(token, "movestogo") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.movestogo = atoi(token);
		}
		else if (strcmp(token, "depth") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.depth = atoi(token);
		}
		else if (strcmp(token, "nodes") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.nodes = (size_t)atoll(token);
		}
		else if (strcmp(token, "mate") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.mate = atoi(token);
		}
		else if (strcmp(token, "perft") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.perft = atoi(token);
		}
		else if (strcmp(token, "movetime") == 0)
		{
			token = strtok(NULL, delim);
			if (token)
				g_goparams.movetime = (clock_t)atoll(token);
		}
		else if (strcmp(token, "infinite") == 0)
			g_goparams.infinite = 1;

		token = strtok(NULL, delim);
	}

	pthread_mutex_unlock(&g_engine_mutex);
	pthread_cond_signal(&g_engine_condvar);
	free(copy);
}
