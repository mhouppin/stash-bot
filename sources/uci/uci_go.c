/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_go.c                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 18:32:36 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 10:36:47 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
