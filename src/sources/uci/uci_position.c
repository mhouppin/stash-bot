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
#include "board.h"
#include "info.h"
#include "uci.h"

void	uci_position(const char *args)
{
	const char			*delim = " \t\n";
	static boardstack_t	**hidden_list = NULL;
	static size_t		hidden_size = 0;
	extern board_t		g_board;
	extern ucioptions_t	g_options;

	pthread_mutex_lock(&g_engine_mutex);
	while (g_engine_mode == THINKING)
	{
		pthread_mutex_unlock(&g_engine_mutex);
		usleep(1000);
		pthread_mutex_lock(&g_engine_mutex);
	}
	pthread_mutex_unlock(&g_engine_mutex);

	if (hidden_size > 0)
	{
		for (size_t i = 0; i < hidden_size; ++i)
			free(hidden_list[i]);
		free(hidden_list);
	}

	hidden_list = malloc(sizeof(boardstack_t *));
	*hidden_list = malloc(sizeof(boardstack_t));
	hidden_size = 1;

	char	*fen;
	char	*copy = strdup(args);
	char	*ptr;
	char	*token = strtok_r(copy, delim, &ptr);

	if (!strcmp(token, "startpos"))
	{
		fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		token = strtok_r(NULL, delim, &ptr);
	}
	else if (!strcmp(token, "fen"))
	{
		fen = strdup("");

		token = strtok_r(NULL, delim, &ptr);

		while (token && strcmp(token, "moves"))
		{
			char	*tmp = malloc(strlen(fen) + strlen(token) + 2);

			strcpy(tmp, fen);
			strcat(tmp, " ");
			strcat(tmp, token);

			free(fen);
			fen = tmp;
			token = strtok_r(NULL, delim, &ptr);
		}
	}
	else
		return ;

	board_set(&g_board, fen, g_options.chess960, *hidden_list);
	free(fen);

	token = strtok_r(NULL, delim, &ptr);

	move_t	move;

	while (token && (move = str_to_move(&g_board, token)) != NO_MOVE)
	{
		hidden_list = realloc(hidden_list,
			sizeof(boardstack_t *) * ++hidden_size);
		hidden_list[hidden_size - 1] = malloc(sizeof(boardstack_t));

		do_move(&g_board, move, hidden_list[hidden_size - 1]);

		token = strtok_r(NULL, delim, &ptr);
	}

	free(copy);
}
