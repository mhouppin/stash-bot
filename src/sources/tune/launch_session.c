#include "info.h"
#include "tune.h"
#include "engine.h"
#include "movelist.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	tuning_pos_t	*pos_list;
	size_t			pos_count;
	double			k;
	double			partial_se;
}		thread_se_t;

score_t	lqsearch(board_t *board, score_t alpha, score_t beta)
{
	if (!board->stack->checkers)
	{
		score_t		stand_pat = evaluate(board);

		if (stand_pat >= beta)
			return (beta);
		if (alpha < stand_pat)
			alpha = stand_pat;
	}

	movelist_t		list;
	boardstack_t	stack;

	list_instable(&list, board);
	generate_move_values(&list, board, NO_MOVE, NULL);

	for (extmove_t *e = list.moves; e < list.last; ++e)
	{
		place_top_move(e, list.last);
		if (!board_legal(board, e->move))
			continue ;

		do_move(board, e->move, &stack);
		score_t	next = -lqsearch(board, -beta, -alpha);
		undo_move(board, e->move);

		if (next >= beta)
			return (beta);
		if (alpha < next)
			alpha = next;
	}
	return (alpha);
}

void	*thread_compute_se(void *ptr)
{
	board_t			board;
	boardstack_t	stack;
	thread_se_t		*data = ptr;
	char			buf[1024];

	for (size_t i = 0; i < data->pos_count; ++i)
	{
		strcpy(buf, data->pos_list[i].fen);
		board_set(&board, buf, false, &stack);

		score_t		lq_score = lqsearch(&board, -INF_SCORE, INF_SCORE);
		double		sigmoid = 1.0 / (1.0 + pow(10.0, -data->k * (double)lq_score / 400.0));

		sigmoid -= data->pos_list[i].svalue;

		data->partial_se += sigmoid * sigmoid;
	}

	return (NULL);
}

double	get_se_value(tuning_session_t *session, int thread_count, double k)
{
	static pthread_t	thread_list[128];
	static thread_se_t	se_list[128];

	for (int i = 0; i < thread_count; ++i)
	{
		size_t	start = session->pos_count * i / thread_count;
		size_t	end = session->pos_count * (i + 1) / thread_count;

		se_list[i].pos_list = session->pos_list + start;
		se_list[i].pos_count = end - start;
		se_list[i].k = k;
		se_list[i].partial_se = 0;

		if (pthread_create(&thread_list[i], NULL, &thread_compute_se, &se_list[i]))
		{
			perror("Unable to create tuning thread");
			exit(EXIT_FAILURE);
		}
	}

	double	se = 0;

	for (int i = 0; i < thread_count; ++i)
	{
		pthread_join(thread_list[i], NULL);
		se += se_list[i].partial_se;
	}

	return (se);
}

void	launch_session(tuning_session_t *session, int max_iterations,
		int thread_count, int ts_debug_level)
{
	if (thread_count > 128 || thread_count < 1)
	{
		fputs("Invalid thread count\n", stderr);
		return ;
	}

	double	k = 0.0;
	double	best_se = 1e30;

	printf("Tuning on " SIZE_FORMAT " positions\n", session->pos_count);

	{
		double	epsilon = 1.0;

		if (ts_debug_level >= TS_Low)
		{
			printf("Computing sigmoid constant K...\n");
			fflush(stdout);
		}

		while (epsilon > 1e-2)
		{
			epsilon *= 0.95;

			double	new_k = k + epsilon;
			double	new_se = get_se_value(session, thread_count, new_k);

			if (new_se < best_se)
			{
				k = new_k;
				best_se = new_se;
				if (ts_debug_level >= TS_Medium)
				{
					printf("new K: %.2lf, SE: %.2lf\n", k, best_se);
					fflush(stdout);
				}
				continue ;
			}

			new_k = k - epsilon;
			new_se = get_se_value(session, thread_count, new_k);

			if (new_se < best_se)
			{
				k = new_k;
				best_se = new_se;
				if (ts_debug_level >= TS_Medium)
				{
					printf("new K: %.2lf, SE: %.2lf\n", k, best_se);
					fflush(stdout);
				}
			}
		}

		if (ts_debug_level >= TS_Low)
		{
			printf("K computation done.\n");
			fflush(stdout);
		}
	}

	int		iteration = 0;
	bool	improving;

	do
	{
		++iteration;

		if (ts_debug_level >= TS_Low)
		{
			printf("Iteration %d\n\n", iteration);
			fflush(stdout);
		}

		improving = false;

		int		dot_counter = 0;

		for (size_t i = 0; i < session->int_count; ++i)
		{
			tuning_int_t	*cur = session->int_list + i;

			if (*cur->value + cur->step <= cur->max)
			{
				*cur->value += cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Increased parameter %s value to %d, SE: %.2lf\n",
							cur->name, *cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value -= cur->step;
			}
			if (*cur->value - cur->step >= cur->min)
			{
				*cur->value -= cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Decreased parameter %s value to %d, SE: %.2lf\n",
							cur->name, *cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value += cur->step;
			}

			if (ts_debug_level >= TS_High)
			{
				putchar('.');

				++dot_counter;
				if (dot_counter == 40 && i + 1 < session->int_count)
				{
					dot_counter = 0;
					putchar('\n');
				}

				fflush(stdout);
			}
		}

		if (ts_debug_level >= TS_High && dot_counter)
			putchar('\n');

		dot_counter = 0;

		for (size_t i = 0; i < session->double_count; ++i)
		{
			tuning_double_t	*cur = session->double_list + i;

			if (*cur->value + cur->step <= cur->max)
			{
				double	tmp = *cur->value;

				*cur->value += cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Increased parameter %s value to %.3lf, SE: %.2lf\n",
							cur->name, *cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value = tmp;
			}
			if (*cur->value - cur->step >= cur->min)
			{
				double	tmp = *cur->value;

				*cur->value -= cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Decreased parameter %s value to %.3lf, SE: %.2lf\n",
							cur->name, *cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value = tmp;
			}

			if (ts_debug_level >= TS_High)
			{
				putchar('.');

				++dot_counter;
				if (dot_counter == 40 && i + 1 < session->double_count)
				{
					dot_counter = 0;
					putchar('\n');
				}

				fflush(stdout);
			}
		}

		if (ts_debug_level >= TS_High && dot_counter)
			putchar('\n');

		dot_counter = 0;

		for (size_t i = 0; i < session->score_count; ++i)
		{
			tuning_score_t	*cur = session->score_list + i;

			if (*cur->value + cur->step <= cur->max)
			{
				*cur->value += cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Increased parameter %s value to %d, SE: %.2lf\n",
							cur->name, (int)*cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value -= cur->step;
			}
			if (*cur->value - cur->step >= cur->min)
			{
				*cur->value -= cur->step;

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Decreased parameter %s value to %d, SE: %.2lf\n",
							cur->name, (int)*cur->value, best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					continue ;
				}
				else
					*cur->value += cur->step;
			}

			if (ts_debug_level >= TS_High)
			{
				putchar('.');

				++dot_counter;
				if (dot_counter == 40 && i + 1 < session->score_count)
				{
					dot_counter = 0;
					putchar('\n');
				}

				fflush(stdout);
			}
		}

		if (ts_debug_level >= TS_High && dot_counter)
			putchar('\n');

		dot_counter = 0;

		for (size_t i = 0; i < session->spair_count; ++i)
		{
			bool	improved_param = false;

			tuning_spair_t	*cur = session->spair_list + i;

			if (midgame_score(*cur->value) + midgame_score(cur->step)
				<= midgame_score(cur->max))
			{
				*cur->value += create_scorepair(midgame_score(cur->step), 0);

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Increased parameter %s value to (%d, %d), SE: %.2lf\n",
							cur->name, (int)midgame_score(*cur->value),
							(int)endgame_score(*cur->value), best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					improved_param = true;
				}
				else
					*cur->value -= create_scorepair(midgame_score(cur->step), 0);
			}
			if (!improved_param
				&& midgame_score(*cur->value) - midgame_score(cur->step)
				>= midgame_score(cur->min))
			{
				*cur->value -= create_scorepair(midgame_score(cur->step), 0);

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Decreased parameter %s value to (%d, %d), SE: %.2lf\n",
							cur->name, (int)midgame_score(*cur->value),
							(int)endgame_score(*cur->value), best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					improved_param = true;
				}
				else
					*cur->value += create_scorepair(midgame_score(cur->step), 0);
			}

			if (ts_debug_level >= TS_High && !improved_param)
			{
				putchar('.');

				++dot_counter;
				if (dot_counter == 40 && i + 1 < session->int_count)
				{
					dot_counter = 0;
					putchar('\n');
				}

				fflush(stdout);
			}

			improved_param = false;

			if (endgame_score(*cur->value) + endgame_score(cur->step)
				<= endgame_score(cur->max))
			{
				*cur->value += create_scorepair(0, endgame_score(cur->step));

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Increased parameter %s value to (%d, %d), SE: %.2lf\n",
							cur->name, (int)midgame_score(*cur->value),
							(int)endgame_score(*cur->value), best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					improved_param = true;
				}
				else
					*cur->value -= create_scorepair(0, endgame_score(cur->step));
			}
			if (!improved_param
				&& endgame_score(*cur->value) - endgame_score(cur->step)
				>= endgame_score(cur->min))
			{
				*cur->value -= create_scorepair(0, endgame_score(cur->step));

				double	new_se = get_se_value(session, thread_count, k);

				if (new_se < best_se)
				{
					best_se = new_se;

					if (ts_debug_level >= TS_Medium)
					{
						if (ts_debug_level >= TS_High && dot_counter)
							putchar('\n');
						printf("Decreased parameter %s value to (%d, %d), SE: %.2lf\n",
							cur->name, (int)midgame_score(*cur->value),
							(int)endgame_score(*cur->value), best_se);
						fflush(stdout);
					}
					improving = true;
					dot_counter = 0;
					improved_param = true;
				}
				else
					*cur->value += create_scorepair(0, endgame_score(cur->step));
			}

			if (ts_debug_level >= TS_High && !improved_param)
			{
				putchar('.');

				++dot_counter;
				if (dot_counter == 40 && i + 1 < session->int_count)
				{
					dot_counter = 0;
					putchar('\n');
				}

				fflush(stdout);
			}
		}

		if (ts_debug_level >= TS_High && dot_counter)
			putchar('\n');

		dot_counter = 0;
	}
	while (improving && (max_iterations <= 0 || iteration < max_iterations));

	printf("Tuning session done. Values:\n");
	dump_item_values(session);
}
