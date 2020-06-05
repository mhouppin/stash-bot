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

#include <stdlib.h>
#include "board.h"
#include "engine.h"
#include "imath.h"
#include "info.h"
#include "movelist.h"
#include "tt.h"

extern int	g_seldepth;

score_t	qsearch(board_t *board, int depth, score_t alpha, score_t beta,
		searchstack_t *ss)
{
	const score_t		old_alpha = alpha;
	movelist_t			list;

	if (g_nodes % 4096 == 0 && out_of_time())
		return (NO_SCORE);

	if (g_seldepth < ss->plies + 1)
		g_seldepth = ss->plies + 1;

	if (is_draw(board, ss->plies + 1))
		return (0);

	// Mate pruning.

	alpha = max(alpha, mated_in(ss->plies));
	beta = min(beta, mate_in(ss->plies + 1));

	if (alpha >= beta)
		return (alpha);

	// Check for interesting tt values

	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);
	score_t		eval = found ? entry->eval : evaluate(board);
	score_t		best_value = eval;

	// If not playing a capture is better because of better quiet moves,
	// allow for a simple eval return.

	if (!board->stack->checkers && alpha < best_value)
	{
		alpha = best_value;
		if (alpha >= beta)
			return (alpha);
	}

	if (found)
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

	move_t	tt_move = entry->bestmove;

	(ss + 1)->plies = ss->plies + 1;

	list_instable(&list, board);
	generate_move_values(&list, board, tt_move, NULL);

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

		score_t		next = -qsearch(board, depth - 1, -beta, -alpha,
			ss + 1);

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
				alpha = best_value;
				bestmove = extmove->move;
				if (alpha >= beta)
					break ;
			}
		}
	}

	if (move_count == 0 && board->stack->checkers)
		best_value = mated_in(ss->plies);

	// Do not erase entries with higher depth for same position.

	if (best_value != NO_SCORE && (entry->key != board->stack->board_key
		|| entry->depth <= -DEPTH_OFFSET))
	{
		int bound = (best_value >= beta) ? LOWER_BOUND
			: (best_value <= old_alpha) ? UPPER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(best_value, ss->plies), eval, 0, bound, bestmove);
	}

	return (best_value);
}
