/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_thread.c                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:22:56 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/28 16:03:15 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const t_cmdlink	commands[] =
{
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
	char	*line = NULL;
	size_t	size = 0;

	while (getline(&line, &size, stdin) > 0)
	{
		char	*cmd;

		cmd = strtok(line, " \t\n");

		for (size_t i = 0; commands[i].cmd_name != NULL; i++)
		{
			if (strcmp(commands[i].cmd_name, cmd))
			{
				commands[i].call(strtok(NULL, ""));
				break ;
			}
		}
	}

	free(line);
	return (NULL);
}
