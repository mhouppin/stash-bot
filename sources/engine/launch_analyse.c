/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   launch_analyse.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 00:05:31 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/01/31 18:45:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

static void	sort_moves(int16_t *g_valuebackup)
{
	size_t	gap, start;
	ssize_t	i;

	for (gap = g_searchmoves->size / 2; gap > 0; gap /= 2)
	{
		for (start = gap; start < g_searchmoves->size; start++)
		{
			for (i = (ssize_t)(start - gap); i >= 0; i = i - gap)
			{
				if (g_real_board.player == PLAYER_WHITE && g_valuemoves[i + gap] <= g_valuemoves[i])
					break ;
				else if (g_real_board.player == PLAYER_BLACK && g_valuemoves[i + gap] >= g_valuemoves[i])
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

					vtmp = g_valuebackup[i];
					g_valuebackup[i] = g_valuebackup[i + gap];
					g_valuebackup[i + gap] = vtmp;
				}
			}
		}
	}
}

clock_t		calc_movetime(clock_t time, clock_t increment)
{
	// if we're already out of time (e.g 0 + 1 matches), only use 2% of timer.

	if (time < increment)
		return (time / 50);
	else
	{
		// Manage our time so that we use either part of the increment, or
		// all the increment if we're early enough.

		clock_t		time_diff = time - increment;

		if (time_diff > increment)
			time_diff = increment;

		return (time / 50 + time_diff);
	}
}

void		launch_analyse(void)
{
	pthread_t	*threads;
	int			*tindex;
	int			i;
	int16_t		value = INT16_MIN;
	int16_t		*g_valuebackup;
	char		*move;
	bool		has_search_aborted;

	if (!g_movetime)
	{
		if (g_real_board.player == PLAYER_WHITE && (g_wtime || g_winc))
		{
			g_movetime = calc_movetime(g_wtime, g_winc);

			if (g_movetime > 60000)
				g_movetime = 60000;
		}
		else if (g_real_board.player == PLAYER_BLACK && (g_btime || g_binc))
		{
			g_movetime = calc_movetime(g_btime, g_binc);

			if (g_movetime > 60000)
				g_movetime = 60000;
		}
	}

	clock_t		limit = ((g_mintime > g_movetime) ? g_mintime : g_movetime);

	g_start = chess_clock();
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
	g_valuebackup = (int16_t *)malloc(2 * g_searchmoves->size);

	for (i = 0; i < g_threads; i++)
		tindex[i] = i;

	for (i = 0; i < (int)g_searchmoves->size; i++)
		g_valuemoves[i] = g_valuebackup[i] = INT16_MIN;

	i = 0;

	g_curnodes = 0;

	pthread_mutex_lock(&mtx_engine);
	while (i < g_depth && (g_infinite || chess_clock() - g_start <= limit))
	{
		pthread_mutex_unlock(&mtx_engine);

		g_curdepth = i;

		for (int k = 0; k < g_threads; k++)
			if (pthread_create(threads + k, NULL, &analysis_thread, tindex + k))
			{
				perror("Unable to initialize engine analysis threads");
				abort();
			}

		for (int k = 0; k < g_threads; k++)
			pthread_join(*(threads + k), NULL);

		has_search_aborted = false;

		for (size_t k = 0; k < g_searchmoves->size; k++)
			if (g_valuemoves[k] == INT16_MIN)
			{
				has_search_aborted = true;
				break ;
			}

		if (has_search_aborted)
		{
			memcpy(g_valuemoves, g_valuebackup, 2 * g_searchmoves->size);
		}
		else
		{
			sort_moves(g_valuebackup);

			value = g_valuemoves[0];

			if (value > -30000 && value < 30000 && i > 0)
				value = (value + g_valuebackup[0]) / 2;
			memcpy(g_valuebackup, g_valuemoves, 2 * g_searchmoves->size);
		}

		move = move_to_str(g_searchmoves->moves[0]);
		{
			clock_t	chess_time = chess_clock() - g_start;
			size_t	chess_nodes = g_curnodes;
			size_t	chess_nps = (!chess_time) ? 0 : (chess_nodes * 1000) / chess_time;

			if (value <= -30000)
			{
				printf("info depth %d nodes %zu nps %zu time %lu score mate %d pv %s\n",
						i - has_search_aborted + 1,
						chess_nodes, chess_nps, chess_time,
						(g_real_board.player == PLAYER_WHITE) ? -(value + 32000)
						: value + 32000, move);
				break ;
			}
			else if (value >= 30000)
			{
				printf("info depth %d nodes %zu nps %zu time %lu score mate %d pv %s\n",
						i - has_search_aborted + 1,
						chess_nodes, chess_nps, chess_time,
						(g_real_board.player == PLAYER_WHITE) ? 32000 - value
						: value - 32000, move);
				break ;
			}
			else
			{
				printf("info depth %d nodes %zu nps %zu time %lu score cp %d pv %s\n",
						i - has_search_aborted + 1,
						chess_nodes, chess_nps, chess_time,
						(g_real_board.player == PLAYER_WHITE) ? value : -value,
						move);
			}
		}

		free(move);

		pthread_mutex_lock(&mtx_engine);
		if (g_engine_send == DO_EXIT || g_curnodes >= g_nodes)
			break ;
		i++;
	}

	move = move_to_str(g_searchmoves->moves[0]);

	printf("bestmove %s\n", move);
	fflush(stdout);
	free(move);
	free(g_valuemoves);
	free(g_valuebackup);
	movelist_quit(g_searchmoves);
	pthread_mutex_unlock(&mtx_engine);
}
