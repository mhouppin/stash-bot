/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_go.c                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 09:36:18 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 06:58:17 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void	uci_go(const char *args)
{
	if (pthread_mutex_lock(&mtx_engine))
	{
		perror("Unable to launch engine");
		return ;
	}

	if (g_engine_mode == THINKING)
	{
		pthread_mutex_unlock(&mtx_engine);
		fputs("Engine is already thinking: please send 'stop' command before"
				" sending 'go' command", stderr);
		return ;
	}

	g_engine_send = DO_THINK;
	g_wtime = NO_TIME;
	g_btime = NO_TIME;
	g_winc = NO_INCREMENT;
	g_binc = NO_INCREMENT;
	g_movestogo = NO_MOVESTOGO;
	g_depth = NO_DEPTH;
	g_nodes = SIZE_MAX;
	g_mate = NO_MATE;
	g_movetime = NO_MOVETIME;
	g_infinite = NO_INFINITE;
	g_searchmoves = NULL;

	char	*copy = strdup(args ? args : "");
	char	*token;

	token = strtok(copy, " \t\n");

	while (token)
	{
		if (strcmp(token, "searchmoves") == 0)
		{
			token = strtok(NULL, " \t\n");
			g_searchmoves = movelist_init();
			while (token)
			{
				push_move(g_searchmoves, str_to_move(token));
				token = strtok(NULL, " \t\n");
			}
			break ;
		}
		else if (strcmp(token, "wtime") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_wtime = (clock_t)atoll(token);
		}
		else if (strcmp(token, "btime") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_btime = (clock_t)atoll(token);
		}
		else if (strcmp(token, "winc") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_winc = (clock_t)atoll(token);
		}
		else if (strcmp(token, "binc") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_binc = (clock_t)atoll(token);
		}
		else if (strcmp(token, "movestogo") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_movestogo = atoi(token);
		}
		else if (strcmp(token, "depth") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_depth = atoi(token);
		}
		else if (strcmp(token, "nodes") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_nodes = (size_t)atoll(token);
		}
		else if (strcmp(token, "mate") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_mate = atoi(token);
		}
		else if (strcmp(token, "movetime") == 0)
		{
			token = strtok(NULL, " \t\n");
			if (token)
				g_movetime = (clock_t)atoll(token) * CLOCKS_PER_SEC / 1000;
		}
		else if (strcmp(token, "infinite") == 0)
			g_infinite = OK_INFINITE;

		token = strtok(NULL, " \t\n");
	}

	pthread_mutex_unlock(&mtx_engine);
	free(copy);
}
