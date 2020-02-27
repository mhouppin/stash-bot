/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine_go.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 21:05:04 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 22:43:49 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "info.h"
#include "uci.h"

clock_t		compute_movetime(clock_t time, clock_t increment, clock_t movestogo)
{
	clock_t		time_estimation = time / movestogo + increment;

	if (time_estimation > time)
		time_estimation = time;

	return (time_estimation);
}

void		engine_go(void)
{
	extern board_t		g_board;
	extern goparams_t	g_goparams;
	extern ucioptions_t	g_options;
	extern movelist_t	g_searchmoves;

	if (movelist_size(&g_searchmoves) == 0)
	{
		if (g_board.stack->checkers)
			printf("info depth 0 score mate 0\nbestmove 0000\n");
		else
			printf("info depth 0 score cp 0\nbestmove 0000\n");
		fflush(stdout);
		return ;
	}

	if (!g_goparams.movetime)
	{
		if (g_goparams.movestogo == 0)
			g_goparams.movestogo = 50;

		if (g_board.side_to_move == WHITE
			&& (g_goparams.wtime || g_goparams.winc))
		{
			g_goparams.movetime = compute_movetime(g_goparams.wtime,
				g_goparams.winc, g_goparams.movestogo);

			if (g_goparams.movetime > 3600000)
				g_goparams.movetime = 3600000;
		}
		else if (g_board.side_to_move == BLACK
			&& (g_goparams.btime || g_goparams.binc))
		{
			g_goparams.movetime = compute_movetime(g_goparams.btime,
				g_goparams.binc, g_goparams.movestogo);

			if (g_goparams.movetime > 3600000)
				g_goparams.movetime = 3600000;
		}
	}

	if (g_options.min_think_time + g_options.move_overhead > g_goparams.movetime)
		g_goparams.movetime = g_options.min_think_time;
	else
		g_goparams.movetime -= g_options.move_overhead;

	clock_t		start = chess_clock();
	score_t		*g_backupscore = (score_t *)malloc(sizeof(score_t)
		* movelist_size(&g_searchmoves));

	for (size_t i = 0; i < movelist_size(&g_searchmoves); ++i)
		g_backupscore[i] = g_searchmoves.moves[i].score = NO_SCORE;

	g_nodes = 0;

	if (g_goparams.depth == 0)
		g_goparams.depth = 255;

	if (g_goparams.nodes == 0)
		g_goparams.nodes = SIZE_MAX;

	size_t		multi_pv = (size_t)g_options.multi_pv;

	if (multi_pv > movelist_size(&g_searchmoves))
		multi_pv = movelist_size(&g_searchmoves);

	for (int iter_depth = 0; iter_depth < g_goparams.depth; ++iter_depth)
	{
		bool	has_search_aborted = false;

		for (size_t pv_line = 0; pv_line < multi_pv; ++pv_line)
		{
			search_bestmove(&g_board, iter_depth, pv_line, start);

			for (size_t k = 0; k < movelist_size(&g_searchmoves); ++k)
				if (abs(g_searchmoves.moves[k].score) > INF_SCORE)
				{
					has_search_aborted = true;
					break ;
				}

			if (has_search_aborted)
			{
				for (size_t k = pv_line; k < movelist_size(&g_searchmoves); ++k)
					g_searchmoves.moves[k].score = g_backupscore[k];
			}
			else
			{
				sort_moves((extmove_t *)movelist_begin(&g_searchmoves)
					+ pv_line, (extmove_t *)movelist_end(&g_searchmoves));

				for (size_t k = pv_line; k < movelist_size(&g_searchmoves); ++k)
					g_backupscore[k] = g_searchmoves.moves[k].score;
			}

			clock_t	chess_time = chess_clock() - start;
			size_t	chess_nodes = g_nodes;
			size_t	chess_nps = (!chess_time) ? 0 : (chess_nodes * 1000)
				/ chess_time;

			printf("info depth %d multipv " SIZE_FORMAT " nodes " SIZE_FORMAT
				" nps " SIZE_FORMAT " time %lu score %s pv %s\n",
				iter_depth - has_search_aborted + 1, pv_line + 1,
				chess_nodes, chess_nps, chess_time,
				score_to_str(g_searchmoves.moves[pv_line].score),
				move_to_str(g_searchmoves.moves[pv_line].move,
					g_board.chess960));

			fflush(stdout);

			if (has_search_aborted)
				break ;
		}

		if (has_search_aborted)
			break ;
	}

	printf("bestmove %s\n", move_to_str(g_searchmoves.moves[0].move,
		g_board.chess960));
	fflush(stdout);
	free(g_backupscore);
}
