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
#include <stdlib.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

score_t	search(board_t *board, int depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	if (depth <= 0)
		return (qsearch(board, depth, alpha, beta, ss));

	worker_t			*worker = get_worker(board);
	movelist_t			list;
	score_t				best_value = -INF_SCORE;

	if (!worker->idx)
		check_time();

	if (worker->seldepth < ss->plies + 1)
		worker->seldepth = ss->plies + 1;

	if (g_engine_send == DO_EXIT || g_engine_send == DO_ABORT
		|| is_draw(board, ss->plies))
		return (0);

	if (ss->plies >= MAX_PLIES)
		return (!board->stack->checkers ? evaluate(board) : 0);

	// Mate pruning.

	alpha = max(alpha, mated_in(ss->plies));
	beta = min(beta, mate_in(ss->plies + 1));

	if (alpha >= beta)
		return (alpha);

	// Check for interesting tt values

	move_t		tt_move = NO_MOVE;
	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);
	score_t		eval;

	if (found)
	{
		score_t	tt_score = score_from_tt(entry->score, ss->plies);
		int		bound = entry->genbound & 3;

		if (entry->depth >= depth - DEPTH_OFFSET)
		{
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
					return (beta);
			}
		}
		tt_move = entry->bestmove;

		eval = ss->static_eval = entry->eval;

		if (bound & (tt_score > eval ? LOWER_BOUND : UPPER_BOUND))
			eval = tt_score;
	}
	else
		eval = ss->static_eval = evaluate(board);

	(ss + 1)->plies = ss->plies + 1;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

	// Razoring.

	if (ss->static_eval + Razor_LightMargin < beta)
	{
		if (depth == 1)
		{
			score_t		max_score = qsearch(board, 0, alpha, beta, ss);
			return (max(ss->static_eval + Razor_LightMargin, max_score));
		}
		if (ss->static_eval + Razor_HeavyMargin < beta && depth <= 3)
		{
			score_t		max_score = qsearch(board, 0, alpha, beta, ss);
			if (max_score < beta)
				return (max(ss->static_eval + Razor_HeavyMargin, max_score));
		}
	}

	// Futility Pruning.

	if (depth <= 2 && eval + 256 * depth <= alpha)
		return (eval);

	// Null move pruning.

	if (depth >= NMP_MinDepth && !board->stack->checkers
		&& ss->plies >= worker->verif_plies
		&& eval >= beta && eval >= ss->static_eval)
	{
		boardstack_t	stack;

		int	nmp_reduction = NMP_BaseReduction
			+ min((eval - beta) / NMP_EvalScale, NMP_MaxEvalReduction)
			+ (depth / 4);

		do_null_move(board, &stack);

		score_t			score = -search(board, depth - nmp_reduction, -beta, -beta + 1,
				ss + 1);

		undo_null_move(board);

		if (score >= beta)
		{
			// Do not trust mate claims.

			if (score > MATE_FOUND)
				score = beta;

			// Do not trust win claims.

			if (worker->verif_plies || (depth <= NMP_TrustDepth && abs(beta) < VICTORY))
				return (score);

			// Zugzwang checking.

			worker->verif_plies = ss->plies + (depth - nmp_reduction) * 3 / 4;

			score_t		zzscore = search(board, depth - nmp_reduction, beta - 1, beta,
				ss);

			worker->verif_plies = 0;

			if (zzscore >= beta)
				return (score);
		}
	}

	if (depth > 7 && !tt_move)
		--depth;

	list_pseudo(&list, board);
	generate_move_values(&list, board, tt_move, ss->killers);

	move_t	bestmove = NO_MOVE;
	int		move_count = 0;
	move_t	quiets[64];
	int		qcount = 0;

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

		if (move_count == 1)
			next = -search(board, depth - 1, -beta, -alpha, ss + 1);
		else
		{
			// Late Move Reductions.

			bool	need_full_depth_search = true;

			if (depth >= LMR_MinDepth && move_count > LMR_MinMoves
				&& !board->stack->checkers)
			{
				int		lmr_depth = depth - (depth + move_count) / 10 - 2;

				next = -search(board, lmr_depth, -alpha - 1, -alpha, ss + 1);

				need_full_depth_search = (alpha < next);
			}

			if (need_full_depth_search)
				next = -search(board, depth - 1, -alpha - 1, -alpha, ss + 1);
		}

		undo_move(board, extmove->move);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
			return (0);

		if (best_value < next)
		{
			best_value = next;

			if (alpha < best_value)
			{
				bestmove = extmove->move;
				alpha = best_value;

				if (alpha >= beta)
				{
					if (!is_capture_or_promotion(board, bestmove))
					{
						int		bonus = (depth <= 12) ? 16 * depth * depth : 20;

						add_history(worker->history,
							piece_on(board, move_from_square(bestmove)),
							bestmove, bonus);

						if (ss->killers[0] == NO_MOVE)
							ss->killers[0] = bestmove;
						else if (ss->killers[0] != bestmove)
							ss->killers[1] = bestmove;

						for (int i = 0; i < qcount; ++i)
							add_history(worker->history,
							piece_on(board, move_from_square(quiets[i])),
							quiets[i], -bonus);
					}

					break ;
				}
			}
		}

		if (qcount < 64 && !is_capture_or_promotion(board, extmove->move))
			quiets[qcount++] = extmove->move;

		if (depth < 4 && qcount > depth * 8)
			break ;
	}

	// Checkmate/Stalemate ?

	if (move_count == 0)
		best_value = (board->stack->checkers) ? mated_in(ss->plies) : 0;

	// Do not erase entries with higher depth for same position.

	if (entry->key != board->stack->board_key || entry->depth <= depth - DEPTH_OFFSET)
	{
		int bound = (best_value >= beta) ? LOWER_BOUND : UPPER_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(best_value, ss->plies),
			ss->static_eval, depth, bound, bestmove);
	}

	return (best_value);
}
