#include "tune.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void		add_tuning_int(tuning_session_t *session,
			const char *name, int *value, int min, int max,
			int step, void (*on_update)(void *))
{
	if (session->int_count == session->int_max_count)
	{
		session->int_max_count += session->int_max_count / 2 + 1;
		session->int_list = realloc(session->int_list,
			sizeof(tuning_int_t) * session->int_max_count);

		if (session->int_list == NULL)
		{
			perror("Failed to extend integer tuning table");
			exit(EXIT_FAILURE);
		}
	}
	session->int_list[session->int_count].name = strdup(name);
	session->int_list[session->int_count].value = value;
	session->int_list[session->int_count].min = min;
	session->int_list[session->int_count].max = max;
	session->int_list[session->int_count].step = step;
	session->int_list[session->int_count].on_update = on_update;
	session->int_count++;
}

void		add_tuning_double(tuning_session_t *session,
			const char *name, double *value, double min, double max,
			double step, void (*on_update)(void *))
{
	if (session->double_count == session->double_max_count)
	{
		session->double_max_count += session->double_max_count / 2 + 1;
		session->double_list = realloc(session->double_list,
			sizeof(tuning_double_t) * session->double_max_count);

		if (session->double_list == NULL)
		{
			perror("Failed to extend floating point tuning table");
			exit(EXIT_FAILURE);
		}
	}
	session->double_list[session->double_count].name = strdup(name);
	session->double_list[session->double_count].value = value;
	session->double_list[session->double_count].min = min;
	session->double_list[session->double_count].max = max;
	session->double_list[session->double_count].step = step;
	session->double_list[session->double_count].on_update = on_update;
	session->double_count++;
}

void		add_tuning_score(tuning_session_t *session,
			const char *name, score_t *value, score_t min, score_t max,
			score_t step, void (*on_update)(void *))
{
	if (session->score_count == session->score_max_count)
	{
		session->score_max_count += session->score_max_count / 2 + 1;
		session->score_list = realloc(session->score_list,
			sizeof(tuning_score_t) * session->score_max_count);

		if (session->score_list == NULL)
		{
			perror("Failed to extend score tuning table");
			exit(EXIT_FAILURE);
		}
	}
	session->score_list[session->score_count].name = strdup(name);
	session->score_list[session->score_count].value = value;
	session->score_list[session->score_count].min = min;
	session->score_list[session->score_count].max = max;
	session->score_list[session->score_count].step = step;
	session->score_list[session->score_count].on_update = on_update;
	session->score_count++;
}

void		add_tuning_scorepair(tuning_session_t *session,
			const char *name, scorepair_t *value, scorepair_t min, scorepair_t max,
			scorepair_t step, void (*on_update)(void *))
{
	if (session->spair_count == session->spair_max_count)
	{
		session->spair_max_count += session->spair_max_count / 2 + 1;
		session->spair_list = realloc(session->spair_list,
			sizeof(tuning_spair_t) * session->spair_max_count);

		if (session->spair_list == NULL)
		{
			perror("Failed to extend scorepair tuning table");
			exit(EXIT_FAILURE);
		}
	}
	session->spair_list[session->spair_count].name = strdup(name);
	session->spair_list[session->spair_count].value = value;
	session->spair_list[session->spair_count].min = min;
	session->spair_list[session->spair_count].max = max;
	session->spair_list[session->spair_count].step = step;
	session->spair_list[session->spair_count].on_update = on_update;
	session->spair_count++;
}
