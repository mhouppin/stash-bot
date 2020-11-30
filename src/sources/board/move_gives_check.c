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

#include "board.h"

bool	move_gives_check(const board_t *board, move_t move)
{
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);

	if (board->stack->check_squares[type_of_piece(piece_on(board, from))]
		& square_bit(to))
		return (true);

	square_t	their_king = board_king_square(board, not_color(board->side_to_move));

	if ((board->stack->king_blockers[not_color(board->side_to_move)]
		& square_bit(from)) && !aligned(from, to, their_king))
		return (true);

	switch (type_of_move(move))
	{
		case NORMAL_MOVE:
			return (false);

		case PROMOTION:
			return (piece_moves(promotion_type(move), to,
				board->piecetype_bits[ALL_PIECES] ^ square_bit(from))
				& square_bit(their_king));

		case EN_PASSANT:
			(void)0;
			square_t	capture_square = create_square(file_of_square(to),
				rank_of_square(from));

			bitboard_t	occupied = (board->piecetype_bits[ALL_PIECES]
				^ square_bit(from) ^ square_bit(capture_square))
				| square_bit(to);

			return ((rook_move_bits(their_king, occupied)
				& pieces_bb(board, board->side_to_move, QUEEN, ROOK))
				| (bishop_move_bits(their_king, occupied)
				& pieces_bb(board, board->side_to_move, QUEEN,
				BISHOP)));

		case CASTLING:
			(void)0;
			square_t	king_from = from;
			square_t	rook_from = to;
			square_t	king_to = relative_square(
				rook_from > king_from ? SQ_G1 : SQ_C1, board->side_to_move);
			square_t	rook_to = relative_square(
				rook_from > king_from ? SQ_F1 : SQ_D1, board->side_to_move);

			return ((PseudoMoves[ROOK][rook_to] & square_bit(their_king))
				&& (rook_move_bits(rook_to, (board->piecetype_bits[ALL_PIECES]
				^ square_bit(king_from) ^ square_bit(rook_from))
				| square_bit(king_to) | square_bit(rook_to))
				& square_bit(their_king)));

		default:
			__builtin_unreachable();
			return (false);
	}
}
