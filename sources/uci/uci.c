/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci.c                                            .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 18:19:46 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:33:50 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const cmdlink_t	commands[] =
{
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

void	*uci_thread(void *nothing __attribute__((unused)))
{
	char	*line = malloc(8192);

	while (fgets(line, 8192, stdin) != NULL)
	{
		char	*cmd;

		cmd = strtok(line, " \t\n");

		if (!cmd)
			continue ;

		for (size_t i = 0; commands[i].cmd_name != NULL; i++)
		{
			if (strcmp(commands[i].cmd_name, cmd) == 0)
			{
				commands[i].call(strtok(NULL, ""));
				break ;
			}
		}

		if (strcmp(cmd, "quit") == 0)
			break ;
	}

	usleep(10000);
	pthread_mutex_lock(&g_engine_mutex);
	while (g_engine_mode != WAITING)
	{
		pthread_mutex_unlock(&g_engine_mutex);
		usleep(1000);
		pthread_mutex_lock(&g_engine_mutex);
	}
	pthread_mutex_unlock(&g_engine_mutex);
	uci_quit(NULL);

	free(line);
	return (NULL);
}

