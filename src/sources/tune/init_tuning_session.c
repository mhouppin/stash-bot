#include "tune.h"

void	init_tuning_session(tuning_session_t *session)
{
	session->int_list = NULL;
	session->int_count = 0;
	session->int_max_count = 0;
	session->double_list = NULL;
	session->double_count = 0;
	session->double_max_count = 0;
	session->score_list = NULL;
	session->score_count = 0;
	session->score_max_count = 0;
	session->spair_list = NULL;
	session->spair_count = 0;
	session->spair_max_count = 0;
	session->pos_list = NULL;
	session->pos_count = 0;
	session->pos_max_count = 0;
}
