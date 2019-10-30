/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_setoption.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:26:41 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 09:57:26 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "settings.h"
#include <string.h>
#include <stdlib.h>

void	uci_setoption(const char *args)
{
	char	*copy = strdup(args);
	char	*token;

	token = strtok(copy, " \t\n");

	if (!token || strcmp(token, "name"))
		goto __end;

	token = strtok(NULL, " \t\n");

	if (!strcmp(token, "Threads"))
	{
		// Skip value token.
		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, " \t\n");

		if (token)
		{
			int value = atoi(token);
			if (value >= 1 && value <= 32)
				g_threads = value;
		}
	}
	else if (!strcmp(token, "Hash"))
	{
		// Skip value token.
		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, " \t\n");
		if (token)
		{
			size_t value = (size_t)atol(token);
			if (value >= 1 && value <= 4096)
				g_hash = value * 1048576ul;
		}
	}
	else if (!strcmp(token, "Clear"))
	{
		//clear_hash();
	}
	else if (!strcmp(token, "MultiPV"))
	{
		// Skip value token.
		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, " \t\n");

		if (token)
		{
			int value = atoi(token);
			if (value >= 1 && value <= 16)
				g_multipv = value;
		}
	}
	else if (!strcmp(token, "Minimum"))
	{
		// Skip Thinking, Time, and value tokens.
		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "Thinking"))
			goto __end;

		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "Time"))
			goto __end;

		token = strtok(NULL, " \t\n");

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, " \t\n");

		if (token)
		{
			clock_t value = (clock_t)atol(token);
			if (value < 30000)
				g_mintime = (value * 1000ul / CLOCKS_PER_SEC);
		}
	}

__end:
	free(copy);
}
