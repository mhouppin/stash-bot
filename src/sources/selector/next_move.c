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

#include "history.h"
#include "selector.h"

void	score_instable(const board_t *board, extmove_t *begin, extmove_t *end)
{
	while (begin < end)
	{
		switch (type_of_move(begin->move))
		{
			case PROMOTION:
				begin->score = 128 + promotion_type(begin->move);
				break ;

			case EN_PASSANT:
				begin->score = 64 + PAWN * 8 - PAWN;
				break ;

			default:
				begin->score = see_greater_than(board, begin->move, 0) ? 64
					: 0;
				begin->score += type_of_piece(piece_on(board,
					move_to_square(begin->move))) * 8;
				begin->score -= type_of_piece(piece_on(board,
					move_from_square(begin->move)));
				break ;
		}
		++begin;
	}
}

void	score_quiet(const board_t *board, extmove_t *begin, extmove_t *end)
{
	const bool	in_endgame = popcount(board->piecetype_bits[ALL_PIECES]) <= 16;

	while (begin < end)
	{
		const square_t		from = move_from_square(begin->move);
		const square_t		to = move_to_square(begin->move);
		const piece_t		piece = piece_on(board, from);
		const scorepair_t	qscore = PsqScore[piece][to] - PsqScore[piece][from];

		begin->score = (in_endgame) ? endgame_score(qscore) : midgame_score(qscore);

		if (board->side_to_move == BLACK)
			begin->score = -begin->score;

		begin->score += get_hist_score(piece, begin->move);

		++begin;
	}
}

move_t	next_move(selector_t *sel)
{
	return (sel->tt_move);
}
