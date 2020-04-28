/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "info.h"
#include "pawns.h"
#include "tt.h"
#include "uci.h"

clock_t		compute_movetime(clock_t time, clock_t increment, clock_t movestogo)
{
	clock_t		time_estimation = time / movestogo + increment;

	if (time_estimation > time)
		time_estimation = time;

	return (time_estimation);
}

uint64_t	perft(board_t *board, unsigned int depth)
{
	if (depth == 0)
		return (1);
	else
	{
		movelist_t	list;
		list_all(&list, board);
		if (depth == 1)
			return (movelist_size(&list));

		uint64_t		sum = 0;

		boardstack_t	stack;

		for (const extmove_t *extmove = movelist_begin(&list);
			extmove < movelist_end(&list); ++extmove)
		{
			do_move(board, extmove->move, &stack);
			sum += perft(board, depth - 1);
			undo_move(board, extmove->move);
		}
		return (sum);
	}
}

void		engine_go(void)
{
	extern board_t		g_board;
	extern goparams_t	g_goparams;
	extern ucioptions_t	g_options;
	extern movelist_t	g_searchmoves;

	g_goparams.start = chess_clock();

	if (g_goparams.perft)
	{
		uint64_t	nodes = perft(&g_board, (unsigned int)g_goparams.perft);

		clock_t		time = chess_clock() - g_goparams.start;

		size_t		nps = (!time) ? 0 : (nodes * 1000) / time;

		printf("info nodes " SIZE_FORMAT " nps " SIZE_FORMAT " time %lu\n",
			(size_t)nodes, nps, time);

		return ;
	}

	if (movelist_size(&g_searchmoves) == 0)
	{
		if (g_board.stack->checkers)
			printf("info depth 0 score mate 0\nbestmove 0000\n");
		else
			printf("info depth 0 score cp 0\nbestmove 0000\n");
		fflush(stdout);
		return ;
	}

	tt_clear();
	reset_pawn_cache();

	g_goparams.initial_max_time = 0;

	if (!g_goparams.movetime)
	{
		if (g_goparams.movestogo == 0)
			g_goparams.movestogo = 50;

		if (g_board.side_to_move == WHITE
			&& (g_goparams.wtime || g_goparams.winc))
		{
			g_goparams.initial_max_time = compute_movetime(g_goparams.wtime,
				g_goparams.winc, g_goparams.movestogo);
		}
		else if (g_board.side_to_move == BLACK
			&& (g_goparams.btime || g_goparams.binc))
		{
			g_goparams.initial_max_time = compute_movetime(g_goparams.btime,
				g_goparams.binc, g_goparams.movestogo);
		}
	}
	else
		g_goparams.initial_max_time = g_goparams.movetime;

	if (g_options.min_think_time + g_options.move_overhead > g_goparams.initial_max_time)
		g_goparams.initial_max_time = g_options.min_think_time;
	else
		g_goparams.initial_max_time -= g_options.move_overhead;

	g_goparams.max_time = g_goparams.initial_max_time;

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
			move_t	pv_list[512];

			search_bestmove(&g_board, iter_depth, pv_line, pv_list);

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

			clock_t	chess_time = chess_clock() - g_goparams.start;
			size_t	chess_nodes = g_nodes;
			size_t	chess_nps = (!chess_time) ? 0 : ((uint64_t)chess_nodes * 1000)
				/ (uint64_t)chess_time;

			extern int	g_seldepth;

			printf("info depth %d seldepth %d multipv " SIZE_FORMAT
				" nodes " SIZE_FORMAT " nps " SIZE_FORMAT " hashfull %d"
				" time %lu score %s pv",
				iter_depth - has_search_aborted + 1, g_seldepth, pv_line + 1,
				chess_nodes, chess_nps, tt_hashfull(), chess_time,
				score_to_str(g_searchmoves.moves[pv_line].score));

			if (has_search_aborted)
				printf(" %s\n", move_to_str(g_searchmoves.moves[pv_line].move,
					g_board.chess960));
			else
			{
				for (size_t i = 0; pv_list[i] != NO_MOVE; ++i)
					printf(" %s", move_to_str(pv_list[i], g_board.chess960));

				puts("");
			}

			fflush(stdout);

			if (has_search_aborted)
				break ;
		}

		if (has_search_aborted)
			break ;

		if (g_goparams.mate < 0 && g_searchmoves.moves[0].score <= mated_in(1 - g_goparams.mate * 2))
			break ;
		if (g_goparams.mate > 0 && g_searchmoves.moves[0].score >= mate_in(g_goparams.mate * 2))
			break ;
	}

	printf("bestmove %s\n", move_to_str(g_searchmoves.moves[0].move,
		g_board.chess960));
	fflush(stdout);
	free(g_backupscore);
}
