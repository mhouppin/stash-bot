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
#include "history.h"
#include "imath.h"
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

void		engine_go(board_t *board)
{
	extern goparams_t	g_goparams;
	extern ucioptions_t	g_options;
	extern movelist_t	g_searchmoves;
	const size_t		root_move_count = movelist_size(&g_searchmoves);

	g_goparams.start = chess_clock();

	if (g_goparams.perft)
	{
		uint64_t	nodes = perft(board, (unsigned int)g_goparams.perft);

		clock_t		time = chess_clock() - g_goparams.start;

		size_t		nps = (!time) ? 0 : (nodes * 1000) / time;

		printf("info nodes " SIZE_FORMAT " nps " SIZE_FORMAT " time %lu\n",
			(size_t)nodes, nps, time);

		return ;
	}

	if (root_move_count == 0)
	{
		if (board->stack->checkers)
			printf("info depth 0 score mate 0\nbestmove 0000\n");
		else
			printf("info depth 0 score cp 0\nbestmove 0000\n");
		fflush(stdout);
		return ;
	}

	// Init root move struct here

	root_move_t		*root_moves = malloc(sizeof(root_move_t) * root_move_count);
	for (size_t i = 0; i < movelist_size(&g_searchmoves); ++i)
	{
		root_moves[i].move = g_searchmoves.moves[i].move;
		root_moves[i].depth = 0;
		root_moves[i].previous_score = root_moves[i].score = -INF_SCORE;
		root_moves[i].pv[0] = NO_MOVE;
	}

	tt_clear();
	reset_pawn_cache();
	reset_history();

	g_goparams.initial_max_time = 0;

	if (!g_goparams.movetime)
	{
		if (g_goparams.movestogo == 0)
		{
			g_goparams.movestogo = 50;

			// If we get enough increment per move to avoid time burns, decrease
			// estimated movestogo.

			if (board->side_to_move == WHITE && g_goparams.winc > g_options.move_overhead)
				g_goparams.movestogo = 35;
			if (board->side_to_move == BLACK && g_goparams.binc > g_options.move_overhead)
				g_goparams.movestogo = 35;
		}

		if (board->side_to_move == WHITE
			&& (g_goparams.wtime || g_goparams.winc))
		{
			g_goparams.initial_max_time = compute_movetime(g_goparams.wtime,
				g_goparams.winc, g_goparams.movestogo);
		}
		else if (board->side_to_move == BLACK
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

	g_nodes = 0;

	if (g_goparams.depth == 0)
		g_goparams.depth = 255;

	if (g_goparams.nodes == 0)
		g_goparams.nodes = SIZE_MAX;

	const int	multi_pv = min(g_options.multi_pv, root_move_count);

	for (int iter_depth = 0; iter_depth < g_goparams.depth; ++iter_depth)
	{
		bool		has_search_aborted = false;
		extern int	g_seldepth;

		g_seldepth = 0;

		for (int pv_line = 0; pv_line < multi_pv; ++pv_line)
		{
			search_bestmove(board, iter_depth, root_moves + pv_line,
				root_moves + root_move_count, pv_line);

			// Catch fail-low and search aborting

			for (root_move_t *i = root_moves; i < root_moves + root_move_count; ++i)
			{
				if (abs(i->score) >= INF_SCORE)
				{
					if (i->score == -NO_SCORE)
						has_search_aborted = true;
					i->score = i->previous_score;
				}
			}

			sort_root_moves(root_moves + pv_line, root_moves + root_move_count);
			sort_root_moves(root_moves, root_moves + multi_pv);

			for (root_move_t *i = root_moves + multi_pv;
				i < root_moves + root_move_count; ++i)
				i->depth = 0;

			clock_t	chess_time = chess_clock() - g_goparams.start;
			size_t	chess_nodes = g_nodes;
			size_t	chess_nps = (!chess_time) ? 0 : ((uint64_t)chess_nodes * 1000)
				/ (uint64_t)chess_time;

			// Don't update Multi-PV lines if not all analysed at current depth
			// and not enough time elapsed

			if (pv_line == multi_pv - 1 || chess_time > 3000)
			{
				for (int i = 0; i < multi_pv; ++i)
				{
					printf("info depth %d seldepth %d multipv %d nodes "
						SIZE_FORMAT " nps " SIZE_FORMAT " hashfull %d"
						" time %lu score %s pv",
						root_moves[i].depth, g_seldepth, i + 1,
						chess_nodes, chess_nps, tt_hashfull(), chess_time,
						score_to_str(root_moves[i].score));
	
					for (size_t k = 0; root_moves[i].pv[k] != NO_MOVE; ++k)
						printf(" %s", move_to_str(root_moves[i].pv[k],
							board->chess960));
					puts("");
				}
				fflush(stdout);
			}

			if (has_search_aborted)
				break ;

			for (root_move_t *i = root_moves; i < root_moves + root_move_count; ++i)
			{
				i->previous_score = i->score;
				i->score = -INF_SCORE;
			}
		}

		if (has_search_aborted)
			break ;

		if (g_goparams.mate < 0 && root_moves->previous_score <= mated_in(1 - g_goparams.mate * 2))
			break ;
		if (g_goparams.mate > 0 && root_moves->previous_score >= mate_in(g_goparams.mate * 2))
			break ;
	}

	// UCI protocol specifies that we shouldn't send the bestmove command
	// before the GUI sends us the "stop" in infinite mode.

	if (g_goparams.infinite)
		while (!out_of_time())
			;

	printf("bestmove %s\n", move_to_str(root_moves->move, board->chess960));
	fflush(stdout);
	free(root_moves);
}
