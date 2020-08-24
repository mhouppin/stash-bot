#include "tune.h"
#include <stdio.h>

void	dump_item_values(tuning_session_t *session)
{
	for (size_t i = 0; i < session->int_count; ++i)
	{
		tuning_int_t	*cur = session->int_list + i;

		printf("--- %24.24s: %d\n", cur->name, *cur->value);
	}
	for (size_t i = 0; i < session->double_count; ++i)
	{
		tuning_double_t	*cur = session->double_list + i;

		printf("--- %24.24s: %.3lf\n", cur->name, *cur->value);
	}
	for (size_t i = 0; i < session->score_count; ++i)
	{
		tuning_score_t	*cur = session->score_list + i;

		printf("--- %24.24s: %d\n", cur->name, (int)*cur->value);
	}
	for (size_t i = 0; i < session->spair_count; ++i)
	{
		tuning_spair_t	*cur = session->spair_list + i;

		printf("--- %24.24s: (%d, %d)\n", cur->name, (int)midgame_score(*cur->value), (int)endgame_score(*cur->value));
	}
	fflush(stdout);
}
