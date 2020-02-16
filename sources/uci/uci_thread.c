/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_thread.c                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:22:56 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/12 20:47:29 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const t_cmdlink	commands[] =
{
	{"d", &uci_d},
	{"debug", &uci_debug},
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
				fflush(stdout);
				break ;
			}
		}

		if (strcmp(cmd, "quit") == 0)
			break ;
	}

	usleep(1000);
	pthread_mutex_lock(&mtx_engine);
	while (g_engine_mode != WAITING)
	{
		pthread_mutex_unlock(&mtx_engine);
		usleep(1000);
		pthread_mutex_lock(&mtx_engine);
	}
	pthread_mutex_unlock(&mtx_engine);

	free(line);
	return (NULL);
}
