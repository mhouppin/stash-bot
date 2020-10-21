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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

int		g_seldepth;

score_t	search_pv(board_t *board, int depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	if (depth <= 0)
		return (qsearch(board, depth, alpha, beta, ss));

	movelist_t			list;
	move_t				pv[512];
	score_t				best_value = -INF_SCORE;

	if (g_nodes % 2048 == 0 && out_of_time())
		return (NO_SCORE);

	if (g_seldepth < ss->plies + 1)
		g_seldepth = ss->plies + 1;

	if (is_draw(board, ss->plies + 1))
		return (0);

	// Check for interesting tt values

	move_t		tt_move = NO_MOVE;
	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);
	score_t		eval;

	if (found)
	{
		eval = entry->eval;
		tt_move = entry->bestmove;
	}
	else
		eval = evaluate(board);

	(ss + 1)->pv = pv;
	pv[0] = NO_MOVE;
	(ss + 1)->plies = ss->plies + 1;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

	list_pseudo(&list, board);
	generate_move_values(&list, board, tt_move, ss->killers);

	move_t	bestmove = NO_MOVE;
	int		move_count = 0;

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		place_top_move((extmove_t *)extmove, (extmove_t *)movelist_end(&list));
		if (!board_legal(board, extmove->move))
			continue ;

		move_count++;

		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next;

		pv[0] = NO_MOVE;

		if (move_count == 1)
			next = -search_pv(board, depth - 1, -beta, -alpha, ss + 1);
		else
		{
			// Late Move Reductions.

			bool	need_full_depth_search = true;

			if (depth >= LMR_MinDepth && move_count > LMR_MinMoves
				&& !board->stack->checkers)
			{
				int		lmr_depth = depth - (depth + move_count) / 10 - 2;

				next = -search(board, lmr_depth, -alpha - 1, -alpha, ss + 1);

				need_full_depth_search = (abs(next) < INF_SCORE && alpha < next);
			}

			if (need_full_depth_search)
			{
				pv[0] = NO_MOVE;

				next = -search(board, depth - 1, -alpha - 1, -alpha, ss + 1);

				if (alpha < next && next < beta)
				{
					pv[0] = NO_MOVE;
					next = -search_pv(board, depth - 1, -beta, -alpha, ss + 1);
				}
			}
		}

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			best_value = NO_SCORE;
			break ;
		}

		if (best_value < next)
		{
			best_value = next;

			if (alpha < best_value)
			{
				ss->pv[0] = bestmove = extmove->move;
				alpha = best_value;

				size_t	j;
				for (j = 0; (ss + 1)->pv[j] != NO_MOVE; ++j)
					ss->pv[j + 1] = (ss + 1)->pv[j];

				ss->pv[j + 1] = NO_MOVE;

				if (alpha >= beta)
				{
					if (!is_capture_or_promotion(board, bestmove))
					{
						add_hist_bonus(piece_on(board,
							move_from_square(bestmove)), bestmove);

						if (ss->killers[0] == NO_MOVE)
							ss->killers[0] = bestmove;
						else if (ss->killers[0] != bestmove)
							ss->killers[1] = bestmove;
					}

					while (--extmove >= movelist_begin(&list))
						if (!is_capture_or_promotion(board, extmove->move))
							add_hist_penalty(piece_on(board,
								move_from_square(extmove->move)), extmove->move);

					break ;
				}
			}
		}
	}

	// Checkmate/Stalemate ?

	if (move_count == 0)
		best_value = (board->stack->checkers) ? mated_in(ss->plies) : 0;

	// Do not erase entries with higher depth for same position.

	if (best_value != NO_SCORE && (entry->key != board->stack->board_key
		|| entry->depth <= depth - DEPTH_OFFSET))
	{
		int bound = (bestmove == NO_MOVE) ? UPPER_BOUND
			: (best_value >= beta) ? LOWER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(best_value, ss->plies), eval, depth, bound, bestmove);
	}

	return (best_value);
}

void	search_bestmove(board_t *board, int depth, score_t alpha, score_t beta,
		root_move_t *begin, root_move_t *end, int pv_line)
{
	extern goparams_t	g_goparams;
	searchstack_t		sstack[512];
	move_t				pv[512];

	memset(sstack, 0, sizeof(sstack));

	sstack[0].plies = 1;
	sstack[0].pv = pv;

	movelist_t			list;
	int					move_count = 0;

	list_pseudo(&list, board);
	generate_move_values(&list, board, begin->move, NULL);

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		root_move_t	*cur;

		place_top_move((extmove_t *)extmove, (extmove_t *)movelist_end(&list));
		if ((cur = find_root_move(begin, end, extmove->move)) == NULL)
			continue ;

		move_count++;

		boardstack_t	stack;

		do_move(board, cur->move, &stack);

		clock_t			elapsed = chess_clock() - g_goparams.start;

		if (elapsed > 3000)
		{
			uint64_t	nps = (g_nodes * 1000) / elapsed;

			printf("info depth %d nodes %lu nps %lu"
				" time %lu currmove %s currmovenumber %d\n",
				depth, (info_t)g_nodes, (info_t)nps, elapsed,
				move_to_str(cur->move, board->chess960),
				move_count + pv_line);
			fflush(stdout);
		}

		pv[0] = NO_MOVE;

		score_t		next;

		if (move_count == 1)
			next = -search_pv(board, depth - 1, -beta, -alpha, sstack);
		else
		{
			// Late Move Reductions.

			bool	need_full_depth_search = true;

			if (depth >= LMR_MinDepth && move_count > LMR_MinMoves
				&& !board->stack->checkers)
			{
				int		lmr_depth = depth - (depth + move_count) / 10 - 2;

				next = -search(board, lmr_depth, -alpha - 1, -alpha, sstack);

				need_full_depth_search = (abs(next) < INF_SCORE && alpha < next);
			}

			if (need_full_depth_search)
			{
				pv[0] = NO_MOVE;

				next = -search(board, depth - 1, -alpha - 1, -alpha, sstack);

				if (alpha < next && next < beta)
				{
					pv[0] = NO_MOVE;
					next = -search_pv(board, depth - 1, -beta, -alpha, sstack);
				}
			}
		}

		undo_move(board, cur->move);

		if (abs(next) > INF_SCORE)
		{
			pthread_mutex_lock(&g_engine_mutex);
			g_engine_send = DO_EXIT;
			pthread_mutex_unlock(&g_engine_mutex);
			return ;
		}
		else if (move_count == 1 || next > alpha)
		{
			cur->score = next;
			alpha = max(alpha, next);
			cur->seldepth = g_seldepth;
			cur->pv[0] = cur->move;

			size_t	j;
			for (j = 0; sstack[0].pv[j] != NO_MOVE; ++j)
				cur->pv[j + 1] = sstack[0].pv[j];

			cur->pv[j + 1] = NO_MOVE;

			if (next >= beta)
				return ;
		}
	}
	return ;
}
