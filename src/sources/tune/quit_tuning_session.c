#include "tune.h"
#include <stdlib.h>

void	quit_tuning_session(tuning_session_t *session)
{
	// Free each FEN in the position list

	for (size_t i = 0; i < session->pos_count; ++i)
		free(session->pos_list[i].fen);

	free(session->pos_list);

	for (size_t i = 0; i < session->int_count; ++i)
		free(session->int_list[i].name);

	free(session->int_list);

	for (size_t i = 0; i < session->double_count; ++i)
		free(session->double_list[i].name);

	free(session->double_list);

	for (size_t i = 0; i < session->score_count; ++i)
		free(session->score_list[i].name);

	free(session->score_list);

	for (size_t i = 0; i < session->spair_count; ++i)
		free(session->spair_list[i].name);

	free(session->spair_list);

	// Just in case someone calls the function more than once.

	init_tuning_session(session);
}
