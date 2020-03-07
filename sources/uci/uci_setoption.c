/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_setoption.c                                  .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 19:58:11 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/07 09:55:25 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "tt.h"
#include "uci.h"
#include <string.h>
#include <stdlib.h>

void	uci_setoption(const char *args)
{
	const char			*delim = " \t\n";
	extern ucioptions_t	g_options;

	char	*copy = strdup(args);
	char	*token = strtok(copy, delim);


	if (!token || strcmp(token, "name"))
		goto __end;

	token = strtok(NULL, delim);

	if (!strcmp(token, "Hash"))
	{
		token = strtok(NULL, delim);

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, delim);

		if (token)
		{
			size_t	value = (size_t)atol(token);
			if (value >= 1 && value <= 4096)
				tt_resize(value);
		}
	}
	else if (!strcmp(token, "Clear"))
	{
		tt_bzero();
	}
	else if (!strcmp(token, "Move"))
	{
		token = strtok(NULL, delim);

		if (!token || strcmp(token, "Overhead"))
			goto __end;

		token = strtok(NULL, delim);

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, delim);

		if (token)
		{
			clock_t		value = (clock_t)atol(token);
			if (value <= 1000)
				g_options.move_overhead = value;
		}
	}
	else if (!strcmp(token, "MultiPV"))
	{
		token = strtok(NULL, delim);

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, delim);

		if (token)
		{
			int value = atoi(token);
			if (value >= 1 && value <= 16)
				g_options.multi_pv = value;
		}
	}
	else if (!strcmp(token, "Minimum"))
	{
		token = strtok(NULL, delim);

		if (!token || strcmp(token, "Thinking"))
			goto __end;

		token = strtok(NULL, delim);

		if (!token || strcmp(token, "Time"))
			goto __end;

		token = strtok(NULL, delim);

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, delim);

		if (token)
		{
			clock_t	value = (clock_t)atol(token);
			if (value <= 30000)
				g_options.min_think_time = value;
		}
	}
	else if (!strcmp(token, "UCI_Chess960"))
	{
		token = strtok(NULL, delim);

		if (!token || strcmp(token, "value"))
			goto __end;

		token = strtok(NULL, delim);

		if (token)
			g_options.chess960 = !strcmp(token, "true");
	}

__end:
	free(copy);
}
