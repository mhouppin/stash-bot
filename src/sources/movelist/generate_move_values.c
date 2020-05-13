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
#include "movelist.h"

void	generate_move_values(movelist_t *movelist, const board_t *board,
		move_t tt_move, move_t *killers)
{
	for (size_t i = 0; i < movelist_size(movelist); ++i)
	{
		move_t		move = movelist->moves[i].move;

		if (move == tt_move)
		{
			movelist->moves[i].score = 8192;
			continue ;
		}

		square_t	from = move_from_square(move);
		square_t	to = move_to_square(move);

		piece_t		moved_piece = piece_on(board, from);
		piece_t		captured_piece = piece_on(board, to);

		if (type_of_move(move) == PROMOTION)
			movelist->moves[i].score = 4096 + PieceScores[ENDGAME][promotion_type(move)];

		else if (type_of_move(move) == EN_PASSANT)
			movelist->moves[i].score = 2048 + PAWN * 8 - PAWN;

		else if (captured_piece != NO_PIECE)
		{
			movelist->moves[i].score = see_greater_than(board, move, 0) ? 2048 : 1024;

			movelist->moves[i].score += type_of_piece(captured_piece) * 8
				- type_of_piece(moved_piece);
		}
		else if (killers && (move == killers[0] || move == killers[1]))
			movelist->moves[i].score = 1536;
		else
		{
			movelist->moves[i].score =
				midgame_score(PsqScore[moved_piece][to])
				- midgame_score(PsqScore[moved_piece][from]);

			if (board->side_to_move == BLACK)
				movelist->moves[i].score = -movelist->moves[i].score;

			movelist->moves[i].score += get_hist_score(moved_piece, move);
		}
	}
}
