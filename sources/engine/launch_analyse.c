/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   launch_analyse.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 00:05:31 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/01 15:31:06 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define BLUNDER_MAX		300
#define BLUNDER_GAP		1200

static void	sort_moves(void)
{
	size_t	gap, start;
	ssize_t	i;

	for (gap = g_searchmoves->size / 2; gap > 0; gap /= 2)
	{
		for (start = gap; start < g_searchmoves->size; start++)
		{
			for (i = (ssize_t)(start - gap); i >= 0; i = i - gap)
			{
				if (g_valuemoves[i + gap] <= g_valuemoves[i])
					break ;
				else
				{
					move_t	mtmp;
					int16_t	vtmp;

					mtmp = g_searchmoves->moves[i];
					g_searchmoves->moves[i] = g_searchmoves->moves[i + gap];
					g_searchmoves->moves[i + gap] = mtmp;

					vtmp = g_valuemoves[i];
					g_valuemoves[i] = g_valuemoves[i + gap];
					g_valuemoves[i + gap] = vtmp;
				}
			}
		}
	}
}

void		launch_analyse(void)
{
	pthread_t	*threads;
	int			*tindex;
	int			i;
	int16_t		value;
	char		*move;

	if (!g_movetime)
	{
		if (g_real_board.player == PLAYER_WHITE && (g_wtime || g_winc))
		{
			g_movetime = g_wtime / 50 + g_winc;
			if (g_movetime > 60000)
				g_movetime = 60000;
		}
		else if (g_real_board.player == PLAYER_BLACK && (g_btime || g_binc))
		{
			g_movetime = g_btime / 50 + g_binc;
			if (g_movetime > 60000)
				g_movetime = 60000;
		}
		g_movetime *= (clock_t)((double)CLOCKS_PER_SEC * sqrt(g_threads));
		g_movetime /= 1000;
	}

	clock_t		limit = ((g_mintime > g_movetime) ? g_mintime : g_movetime);

	g_start = clock();
	threads = (pthread_t *)malloc(sizeof(pthread_t) * g_threads);
	tindex = (int *)malloc(sizeof(int) * g_threads);

	if (g_searchmoves == NULL)
		g_searchmoves = get_simple_moves(&g_real_board);

	if (g_searchmoves->size == 0)
	{
		fprintf(stderr, "Error, already mated\n");
		fflush(stderr);
		return ;
	}

	g_valuemoves = (int16_t *)malloc(2 * g_searchmoves->size);

	for (i = 0; i < g_threads; i++)
		tindex[i] = i;

	i = 0;

	pthread_mutex_lock(&mtx_engine);
	while (i < g_depth && (g_infinite || clock() - g_start <= limit))
	{
		pthread_mutex_unlock(&mtx_engine);

		if (g_searchmoves->size == 1)
			break ;

		g_curdepth = i;

		for (int k = 0; k < g_threads; k++)
			if (pthread_create(threads + k, NULL, &analysis_thread, tindex + k))
			{
				perror("Unable to initialize engine analysis threads");
				abort();
			}

		for (int k = 0; k < g_threads; k++)
			pthread_join(*(threads + k), NULL);

		sort_moves();

		if (g_real_board.player == PLAYER_BLACK)
		{
			value = g_valuemoves[g_searchmoves->size - 1];
			move = move_to_str(g_searchmoves->moves[g_searchmoves->size - 1]);
		}
		else
		{
			value = g_valuemoves[0];
			move = move_to_str(g_searchmoves->moves[0]);
		}

		if (value <= -15000)
		{
			printf("info depth %d time %lu score mate %d pv %s\n", i,
				(clock() - g_start) * 1000 / CLOCKS_PER_SEC,
				(g_real_board.player == PLAYER_WHITE) ? -(value + 16000)
				: value + 16000, move);
			break ;
		}
		else if (value >= 15000)
		{
			printf("info depth %d time %lu score mate %d pv %s\n", i,
				(clock() - g_start) * 1000 / CLOCKS_PER_SEC,
				(g_real_board.player == PLAYER_WHITE) ? 16000 - value
				: value - 16000, move);
			break ;
		}
		else
		{
			printf("info depth %d time %lu score cp %d pv %s\n", i,
				(clock() - g_start) * 1000 / CLOCKS_PER_SEC,
				(g_real_board.player == PLAYER_WHITE) ? value : -value,
				move);
		}

		free(move);

		int16_t		max_diff = (BLUNDER_MAX + (double)BLUNDER_GAP / sqrt(i + 1));

		if (g_real_board.player == PLAYER_WHITE)
		{
			size_t	k = 0;
			while (k < g_searchmoves->size)
			{
				if (g_valuemoves[k] < value - max_diff)
				{
					move = move_to_str(g_searchmoves->moves[k]);
					printf("Popped move %s (value was %hd)\n", move,
							g_valuemoves[k]);
					free(move);
					pop_move(g_searchmoves, k);
					memcpy(g_valuemoves + k, g_valuemoves + k + 1,
							g_searchmoves->size - k);
				}
				else
					k++;
			}
		}
		else
		{
			size_t	k = 0;
			while (k < g_searchmoves->size)
			{
				if (g_valuemoves[k] > value + max_diff)
				{
					move = move_to_str(g_searchmoves->moves[k]);
					printf("Popped move %s (value was %hd)\n", move,
							g_valuemoves[k]);
					free(move);
					pop_move(g_searchmoves, k);
					memcpy(g_valuemoves + k, g_valuemoves + k + 1,
							g_searchmoves->size - k);
				}
				else
					k++;
			}
		}

		pthread_mutex_lock(&mtx_engine);
		if (g_engine_send == DO_EXIT)
			break ;
		i++;
	}

	if (g_real_board.player == PLAYER_BLACK)
		move = move_to_str(g_searchmoves->moves[g_searchmoves->size - 1]);
	else
		move = move_to_str(g_searchmoves->moves[0]);

	printf("bestmove %s\n", move);
	fflush(stdout);
	free(move);
	free(g_valuemoves);
	movelist_quit(g_searchmoves);
	pthread_mutex_unlock(&mtx_engine);
}
