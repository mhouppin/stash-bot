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
#include "lazy_smp.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

score_t	qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss)
{
	worker_t			*worker = get_worker(board);
	const score_t		old_alpha = alpha;
	movelist_t			list;

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
		int			bound = entry->genbound & 3;
		score_t		tt_score = score_from_tt(entry->score, ss->plies);

		if (bound == EXACT_BOUND
			|| (bound == LOWER_BOUND && tt_score >= beta)
			|| (bound == UPPER_BOUND && tt_score <= alpha))
			return (tt_score);
	}

	move_t	tt_move = entry->bestmove;

	(ss + 1)->plies = ss->plies + 1;

	list_instable(&list, board);
	generate_move_values(&list, board, tt_move, NULL);

	move_t	bestmove = NO_MOVE;
	int		move_count = 0;

	// Check if delta pruning is possible.

	const bool		delta_pruning = (!board->stack->checkers
		&& popcount(board->piecetype_bits[ALL_PIECES]) > 6);
	const score_t	delta_base = eval + PAWN_EG_SCORE * 2;

	for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
	{
		place_top_move(extmove, list.last);
		const move_t	currmove = extmove->move;

		if (!board_legal(board, currmove))
			continue ;

		move_count++;

		bool	gives_check = move_gives_check(board, currmove);

		if (delta_pruning && !gives_check
			&& type_of_move(currmove) == NORMAL_MOVE)
		{
			score_t		delta = delta_base + PieceScores[ENDGAME]
				[type_of_piece(piece_on(board, move_to_square(currmove)))];

			// Check if the move is very unlikely to improve alpha.

			if (delta < alpha)
				continue ;
		}

		boardstack_t	stack;

		do_move_gc(board, currmove, &stack, gives_check);

		score_t		next = -qsearch(board, -beta, -alpha, ss + 1);

		undo_move(board, currmove);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
			return (0);

		if (best_value < next)
		{
			best_value = next;
			if (alpha < best_value)
			{
				alpha = best_value;
				bestmove = currmove;
				if (alpha >= beta)
					break ;
			}
		}
	}

	if (move_count == 0 && board->stack->checkers)
		best_value = mated_in(ss->plies);

	// Do not erase entries with higher depth for same position.

	if (entry->key != board->stack->board_key || entry->depth == 0)
	{
		int bound = (best_value >= beta) ? LOWER_BOUND
			: (best_value <= old_alpha) ? UPPER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, score_to_tt(best_value, ss->plies), eval, 0, bound, bestmove);
	}

	return (best_value);
}
