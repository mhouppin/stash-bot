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
#include "info.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

int		g_seldepth;

score_t	qsearch(board_t *board, int max_depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	movelist_t			list;

	if (g_nodes % 4096 == 0 && out_of_time())
		return (NO_SCORE);

	if (g_seldepth < ss->plies + 1)
		g_seldepth = ss->plies + 1;

	list_instable(&list, board);

	// If not playing a capture is better because of better quiet moves,
	// allow for a simple eval return.

	if (!board->stack->checkers)
	{
		score_t	eval = evaluate(board);

		if (alpha < eval)
		{
			alpha = eval;
			if (alpha >= beta)
				return (alpha);
		}
	}

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(ss->plies + 1));
		else
			return (alpha);
	}

	if (is_draw(board, ss->plies + 1))
		return (0);

	// Check for interesting tt values

	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);

	if (found)
	{
		extern transposition_t	g_hashtable;

		int	bound = entry->genbound & 3;

		score_t		tt_score = score_from_tt(entry->score, ss->plies);

		if (bound == EXACT_BOUND)
			return (tt_score);
		else if (bound == LOWER_BOUND && tt_score > alpha)
		{
			alpha = tt_score;
			if (alpha >= beta)
				return (alpha);
		}
		else if (bound == UPPER_BOUND && tt_score < beta)
		{
			beta = tt_score;
			if (alpha >= beta)
				return (alpha);
		}
	}

	move_t	tt_move = entry->bestmove;

	(ss + 1)->plies = ss->plies + 1;

	generate_move_values(&list, board, tt_move, NULL);
	sort_moves((extmove_t *)movelist_begin(&list),
		(extmove_t *)movelist_end(&list));

	move_t	bestmove = NO_MOVE;

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next = -qsearch(board, max_depth - 1, -beta, -alpha,
			ss + 1);

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			alpha = NO_SCORE;
			break ;
		}

		if (alpha < next)
		{
			alpha = next;
			bestmove = extmove->move;
		}

		if (alpha >= beta)
			break ;
	}

	// Do not erase entries with higher depth for same position.

	if (alpha != NO_SCORE && (entry->key != board->stack->board_key
		|| entry->depth <= -DEPTH_OFFSET))
	{
		int bound = (bestmove == NO_MOVE) ? UPPER_BOUND
			: (alpha >= beta) ? LOWER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(alpha, ss->plies), 0, bound, bestmove);
	}

	return (alpha);
}

score_t	search(board_t *board, int max_depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	if (max_depth <= 0)
		return (qsearch(board, max_depth, alpha, beta, ss));

	movelist_t			list;
	move_t				pv[512];

	if (g_nodes % 4096 == 0 && out_of_time())
		return (NO_SCORE);

	if (g_seldepth < ss->plies + 1)
		g_seldepth = ss->plies + 1;

	// Mate pruning.

	{
		score_t		mate_alpha = mated_in(ss->plies);
		score_t		mate_beta = mate_in(ss->plies + 1);

		if (alpha < mate_alpha)
			alpha = mate_alpha;
		if (beta > mate_beta)
			beta = mate_beta;

		if (alpha >= beta)
			return (alpha);
	}

	list_all(&list, board);

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(ss->plies));
		else
			return (0);
	}

	if (is_draw(board, ss->plies + 1))
		return (0);

	// Check for interesting tt values

	move_t		tt_move = NO_MOVE;
	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);

	if (found)
	{
		extern transposition_t	g_hashtable;

		if (entry->depth >= max_depth - DEPTH_OFFSET)
		{
			int	bound = entry->genbound & 3;

			score_t		tt_score = score_from_tt(entry->score, ss->plies);

			if (bound == EXACT_BOUND)
				return (tt_score);
			else if (bound == LOWER_BOUND && tt_score > alpha)
			{
				alpha = tt_score;
				if (alpha >= beta)
					return (alpha);
			}
			else if (bound == UPPER_BOUND && tt_score < beta)
			{
				beta = tt_score;
				if (alpha >= beta)
					return (alpha);
			}
		}
		tt_move = entry->bestmove;
	}

	(ss + 1)->pv = pv;
	pv[0] = NO_MOVE;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

	score_t		eval = evaluate(board);

	// Razoring.

	if (eval + Razor_LightMargin < beta)
	{
		if (max_depth == 1)
		{
			score_t		max_score = qsearch(board, 0, alpha, beta, ss);
			return (eval + Razor_LightMargin < max_score ? max_score : eval + Razor_LightMargin);
		}
		if (eval + Razor_HeavyMargin < beta && max_depth <= 3)
		{
			score_t		max_score = qsearch(board, 0, alpha, beta, ss);
			if (max_score < beta)
				return (eval + Razor_HeavyMargin < max_score ? max_score : eval + Razor_HeavyMargin);
		}
	}

	// Null move pruning.

	if (max_depth >= NMP_MinDepth && !board->stack->checkers
		&& board->stack->plies_from_null_move >= NMP_MinPlies
		&& eval >= beta)
	{
		boardstack_t	stack;

		int		nmp_reduction = (eval - beta) / NMP_EvalScale;

		if (nmp_reduction > NMP_MaxEvalReduction)
			nmp_reduction = NMP_MaxEvalReduction;

		nmp_reduction += NMP_BaseReduction + (max_depth / 4);

		do_null_move(board, &stack);

		(ss + 1)->plies = ss->plies;

		score_t			score = -search(board, max_depth - nmp_reduction, -beta, -beta + 1,
				ss + 1);

		undo_null_move(board);

		if (abs(score) > INF_SCORE)
			return (NO_SCORE);

		if (score >= beta)
		{
			// Do not trust mate claims.

			if (score > MATE_FOUND)
				score = beta;

			// Do not trust win claims.

			if (max_depth <= NMP_TrustDepth && abs(beta) < NMP_TrustScore)
				return (score);

			// Zugzwang checking.

			int nmp_depth = board->stack->plies_from_null_move;
			board->stack->plies_from_null_move = -(max_depth - nmp_reduction) * 3 / 4;

			score_t		zzscore = search(board, max_depth - nmp_reduction, beta - 1, beta,
					ss + 1);

			board->stack->plies_from_null_move = nmp_depth;

			if (zzscore >= beta)
				return (score);
		}
	}

	(ss + 1)->plies = ss->plies + 1;

	generate_move_values(&list, board, tt_move, ss->killers);
	sort_moves((extmove_t *)movelist_begin(&list),
		(extmove_t *)movelist_end(&list));

	move_t	bestmove = NO_MOVE;

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next;

		pv[0] = NO_MOVE;

		if (extmove == movelist_begin(&list))
			next = -search(board, max_depth - 1, -beta, -alpha,
				ss + 1);
		else
		{
			// Late Move Reductions.

			bool	need_full_depth_search = true;

			if (max_depth >= LMR_MinDepth && extmove >= movelist_begin(&list) + LMR_MinMoves
				&& !board->stack->checkers)
			{

				int		lmr_depth = max_depth - 1 - ilogb(max_depth);

				next = -search(board, lmr_depth, -alpha - 1, -alpha,
					ss + 1);

				need_full_depth_search = (abs(next) <= INF_SCORE && alpha < next);
			}

			if (need_full_depth_search)
			{
				pv[0] = NO_MOVE;

				next = -search(board, max_depth - 1, -alpha - 1, -alpha,
					ss + 1);

				if (alpha < next && next < beta)
				{
					pv[0] = NO_MOVE;
					next = -search(board, max_depth - 1, -beta, -next,
						ss + 1);
				}
			}
		}

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			alpha = NO_SCORE;
			break ;
		}

		if (alpha < next)
		{
			ss->pv[0] = bestmove = extmove->move;
			alpha = next;

			size_t	j;
			for (j = 0; (ss + 1)->pv[j] != NO_MOVE; ++j)
				ss->pv[j + 1] = (ss + 1)->pv[j];

			ss->pv[j + 1] = NO_MOVE;
		}

		if (alpha >= beta)
		{
			if (!is_capture_or_promotion(board, bestmove))
			{
				if (ss->killers[0] == NO_MOVE)
					ss->killers[0] = bestmove;
				else if (ss->killers[0] != bestmove)
					ss->killers[1] = bestmove;
			}
			break ;
		}
	}

	// Do not erase entries with higher depth for same position.

	if (alpha != NO_SCORE && (entry->key != board->stack->board_key
		|| entry->depth <= max_depth - DEPTH_OFFSET))
	{
		int bound = (bestmove == NO_MOVE) ? UPPER_BOUND
			: (alpha >= beta) ? LOWER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(alpha, ss->plies), max_depth, bound, bestmove);
	}

	return (alpha);
}

score_t	search_pv(board_t *board, int max_depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	if (max_depth <= 0)
		return (qsearch(board, max_depth, alpha, beta, ss));

	movelist_t			list;
	move_t				pv[512];

	if (g_nodes % 4096 == 0 && out_of_time())
		return (NO_SCORE);

	if (g_seldepth < ss->plies + 1)
		g_seldepth = ss->plies + 1;

	// Mate pruning.

	{
		score_t		mate_alpha = mated_in(ss->plies);
		score_t		mate_beta = mate_in(ss->plies + 1);

		if (alpha < mate_alpha)
			alpha = mate_alpha;
		if (beta > mate_beta)
			beta = mate_beta;

		if (alpha >= beta)
			return (alpha);
	}

	list_all(&list, board);

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(ss->plies));
		else
			return (0);
	}

	if (is_draw(board, ss->plies + 1))
		return (0);

	// Check for interesting tt values

	move_t		tt_move = NO_MOVE;
	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);

	if (found)
		tt_move = entry->bestmove;

	(ss + 1)->pv = pv;
	pv[0] = NO_MOVE;
	(ss + 1)->plies = ss->plies + 1;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

	generate_move_values(&list, board, tt_move, ss->killers);
	sort_moves((extmove_t *)movelist_begin(&list),
		(extmove_t *)movelist_end(&list));

	move_t	bestmove = NO_MOVE;

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next;

		pv[0] = NO_MOVE;

		if (extmove == movelist_begin(&list))
			next = -search_pv(board, max_depth - 1, -beta, -alpha,
				ss + 1);
		else
		{
			// Late Move Reductions.

			bool	need_full_depth_search = true;

			if (max_depth >= LMR_MinDepth && extmove >= movelist_begin(&list) + LMR_MinMoves
				&& !board->stack->checkers)
			{
				int		lmr_depth = max_depth - 1 - ilogb(max_depth);

				next = -search(board, lmr_depth, -alpha - 1, -alpha,
					ss + 1);

				need_full_depth_search = (abs(next) <= INF_SCORE && alpha < next);
			}

			if (need_full_depth_search)
			{
				pv[0] = NO_MOVE;

				next = -search(board, max_depth - 1, -alpha - 1, -alpha,
					ss + 1);

				if (alpha < next && next < beta)
				{
					pv[0] = NO_MOVE;
					next = -search_pv(board, max_depth - 1, -beta, -next,
						ss + 1);
				}
			}
		}

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			alpha = NO_SCORE;
			break ;
		}

		if (alpha < next)
		{
			ss->pv[0] = bestmove = extmove->move;
			alpha = next;

			size_t	j;
			for (j = 0; (ss + 1)->pv[j] != NO_MOVE; ++j)
				ss->pv[j + 1] = (ss + 1)->pv[j];

			ss->pv[j + 1] = NO_MOVE;
		}

		if (alpha >= beta)
		{
			if (!is_capture_or_promotion(board, bestmove))
			{
				if (ss->killers[0] == NO_MOVE)
					ss->killers[0] = bestmove;
				else if (ss->killers[0] != bestmove)
					ss->killers[1] = bestmove;
			}
			break ;
		}
	}

	// Do not erase entries with higher depth for same position.

	if (alpha != NO_SCORE && (entry->key != board->stack->board_key
		|| entry->depth <= max_depth - DEPTH_OFFSET))
	{
		int bound = (bestmove == NO_MOVE) ? UPPER_BOUND
			: (alpha >= beta) ? LOWER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(alpha, ss->plies), max_depth, bound, bestmove);
	}

	return (alpha);
}

void	search_bestmove(board_t *board, int depth, size_t pv_line,
		move_t *display_pv)
{
	extern movelist_t	g_searchmoves;
	extern goparams_t	g_goparams;
	score_t				alpha = -INF_SCORE;
	searchstack_t		sstack[512];
	move_t				pv[512];

	memset(sstack, 0, sizeof(sstack));

	sstack[0].plies = 1;
	sstack[0].pv = pv;
	g_seldepth = 0;

	for (size_t i = pv_line; i < movelist_size(&g_searchmoves); ++i)
	{
		pthread_mutex_lock(&g_engine_mutex);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			g_searchmoves.moves[i].score = NO_SCORE;
			pthread_mutex_unlock(&g_engine_mutex);
			return ;
		}
		pthread_mutex_unlock(&g_engine_mutex);

		boardstack_t	stack;

		do_move(board, g_searchmoves.moves[i].move, &stack);

		clock_t			elapsed = chess_clock() - g_goparams.start;

		if (elapsed > 3000)
		{
			size_t	nps = g_nodes * 1000ul / (size_t)elapsed;

			printf("info depth %d nodes " SIZE_FORMAT " nps " SIZE_FORMAT
				" time %lu currmove %s currmovenumber " SIZE_FORMAT "\n",
				depth + 1, (size_t)g_nodes, nps, elapsed,
				move_to_str(g_searchmoves.moves[i].move, board->chess960),
				i + 1);
			fflush(stdout);
		}

		pv[0] = NO_MOVE;

		if (i == 0)
			g_searchmoves.moves[i].score = -search_pv(board, depth, -INF_SCORE,
				INF_SCORE, sstack);
		else
		{
			g_searchmoves.moves[i].score = -search(board, depth, -alpha - 1,
				-alpha, sstack);

			if (alpha < g_searchmoves.moves[i].score)
			{
				pv[0] = NO_MOVE;
				g_searchmoves.moves[i].score = -search_pv(board, depth,
					-INF_SCORE, -g_searchmoves.moves[i].score, sstack);
			}
		}

		undo_move(board, g_searchmoves.moves[i].move);

		if (abs(g_searchmoves.moves[i].score) > INF_SCORE)
			return ;
		else if (g_searchmoves.moves[i].score > alpha)
		{
			alpha = g_searchmoves.moves[i].score;

			display_pv[0] = g_searchmoves.moves[i].move;

			size_t	j;
			for (j = 0; sstack[0].pv[j] != NO_MOVE; ++j)
				display_pv[j + 1] = sstack[0].pv[j];

			display_pv[j + 1] = NO_MOVE;
		}
		else if (g_searchmoves.moves[i].score == alpha)
			g_searchmoves.moves[i].score -= 1;
	}
	return ;
}
