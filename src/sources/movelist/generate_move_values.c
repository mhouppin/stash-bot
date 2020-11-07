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
#include "lazy_smp.h"
#include "movelist.h"

void	generate_move_values(movelist_t *movelist, const board_t *board,
		move_t tt_move, move_t *killers)
{
	worker_t	*const worker = get_worker(board);
	extmove_t	*const end = movelist->last;

	for (extmove_t *extmove = movelist->moves; extmove < end; ++extmove)
	{
		move_t		move = extmove->move;

		if (move == tt_move)
		{
			extmove->score = 8192;
			continue ;
		}

		switch (type_of_move(move))
		{
			case PROMOTION:
				extmove->score = 4096 + PieceScores[ENDGAME][promotion_type(move)];
				break ;

			case EN_PASSANT:
				extmove->score = 2048 + PAWN * 8 - PAWN;
				break ;

			default: ; // Statement issue
				const square_t	from = move_from_square(move);
				const square_t	to = move_to_square(move);
				const piece_t	moved_piece = piece_on(board, from);
				const piece_t	captured_piece = piece_on(board, to);

				if (captured_piece != NO_PIECE)
				{
					extmove->score = see_greater_than(board, move, -30) ? 2048 : 1024;
					extmove->score += type_of_piece(captured_piece) * 8
						- type_of_piece(moved_piece);
				}
				else if (killers && (move == killers[0] || move == killers[1]))
					extmove->score = 1536;
				else
					extmove->score = get_history_score(worker->history, moved_piece, move);
				break ;
		}
	}
}
