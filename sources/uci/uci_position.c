/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_position.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 20:20:39 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 11:41:44 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
