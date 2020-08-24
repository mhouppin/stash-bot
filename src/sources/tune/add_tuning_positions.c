#include "tune.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void	add_tuning_positions(tuning_session_t *session,
		const char *filename)
{
	FILE	*f;
	char	line_buffer[1024];

	f = fopen(filename, "r");

	if (f == NULL)
	{
		perror("Unable to open position file");
		exit(EXIT_FAILURE);
	}

	while (fgets(line_buffer, 1024, f))
	{
		char	*last_space = strrchr(line_buffer, ' ');

		*last_space = '\0';

		double	cur_svalue = -1.0;

		if (sscanf(last_space + 1, "%lf", &cur_svalue) != 1)
		{
			fputs("Unable to read sigmoid value in position file\n", stderr);
			exit(EXIT_FAILURE);
		}

		char	*cur_fen = strdup(line_buffer);

		if (cur_fen == NULL)
		{
			perror("Unable to load position file");
			exit(EXIT_FAILURE);
		}

		if (session->pos_count == session->pos_max_count)
		{
			session->pos_max_count += session->pos_max_count / 2 + 1;

			session->pos_list = realloc(session->pos_list,
				sizeof(tuning_pos_t) * session->pos_max_count);

			if (session->pos_list == NULL)
			{
				perror("Unable to load position file");
				exit(EXIT_FAILURE);
			}
		}

		session->pos_list[session->pos_count].fen = cur_fen;
		session->pos_list[session->pos_count].svalue = cur_svalue;
		session->pos_count++;
	}
}
