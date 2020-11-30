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

#include "movelist.h"

extmove_t	*create_classic_promotions(extmove_t *movelist, square_t to,
			direction_t direction)
{
	(movelist++)->move = create_promotion(to - direction, to, QUEEN);
	(movelist++)->move = create_promotion(to - direction, to, ROOK);
	(movelist++)->move = create_promotion(to - direction, to, BISHOP);
	(movelist++)->move = create_promotion(to - direction, to, KNIGHT);

	return (movelist);
}

extmove_t	*generate_classic_white_pawn_moves(extmove_t *movelist,
			const board_t *board)
{
	bitboard_t		empty_squares;
	bitboard_t		pawns_on_rank_7 = board->color_bits[WHITE]
		& board->piecetype_bits[PAWN] & RANK_7_BITS;
	bitboard_t		pawns_not_on_rank_7 = board->color_bits[WHITE]
		& board->piecetype_bits[PAWN] & ~RANK_7_BITS;

	bitboard_t		enemies = board->color_bits[BLACK];

	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	bitboard_t		push = shift_up(pawns_not_on_rank_7) & empty_squares;
	bitboard_t		push2 = shift_up(push & RANK_3_BITS) & empty_squares;
	
	while (push)
	{
		square_t	to = pop_first_square(&push);
		(movelist++)->move = create_move(to - NORTH, to);
	}

	while (push2)
	{
		square_t	to = pop_first_square(&push2);
		(movelist++)->move = create_move(to - NORTH * 2, to);
	}

	if (pawns_on_rank_7)
	{
		bitboard_t	promote = shift_up(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_up_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_up_right(pawns_on_rank_7) & enemies;

		while (promote)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote), NORTH);

		while (promote_left)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote_left), NORTH_WEST);

		while (promote_right)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote_right), NORTH_EAST);
	}

	bitboard_t	capture_left = shift_up_left(pawns_not_on_rank_7) & enemies;
	bitboard_t	capture_right = shift_up_right(pawns_not_on_rank_7) & enemies;

	while (capture_left)
	{
		square_t	to = pop_first_square(&capture_left);
		(movelist++)->move = create_move(to - NORTH_WEST, to);
	}

	while (capture_right)
	{
		square_t	to = pop_first_square(&capture_right);
		(movelist++)->move = create_move(to - NORTH_EAST, to);
	}

	if (board->stack->en_passant_square != SQ_NONE)
	{
		bitboard_t	capture_en_passant = pawns_not_on_rank_7
			& pawn_moves(board->stack->en_passant_square, BLACK);

		while (capture_en_passant)
			(movelist++)->move = create_en_passant(
				pop_first_square(&capture_en_passant),
				board->stack->en_passant_square);
	}

	return (movelist);
}

extmove_t	*generate_classic_black_pawn_moves(extmove_t *movelist,
			const board_t *board)
{
	bitboard_t		empty_squares;
	bitboard_t		pawns_on_rank_7 = board->color_bits[BLACK]
		& board->piecetype_bits[PAWN] & RANK_2_BITS;
	bitboard_t		pawns_not_on_rank_7 = board->color_bits[BLACK]
		& board->piecetype_bits[PAWN] & ~RANK_2_BITS;

	bitboard_t		enemies = board->color_bits[WHITE];

	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	bitboard_t		push = shift_down(pawns_not_on_rank_7) & empty_squares;
	bitboard_t		push2 = shift_down(push & RANK_6_BITS) & empty_squares;

	while (push)
	{
		square_t	to = pop_first_square(&push);
		(movelist++)->move = create_move(to - SOUTH, to);
	}

	while (push2)
	{
		square_t	to = pop_first_square(&push2);
		(movelist++)->move = create_move(to - SOUTH * 2, to);
	}

	if (pawns_on_rank_7)
	{
		bitboard_t	promote = shift_down(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_down_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_down_right(pawns_on_rank_7) & enemies;

		while (promote)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote), SOUTH);

		while (promote_left)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote_left), SOUTH_WEST);

		while (promote_right)
			movelist = create_classic_promotions(movelist,
				pop_first_square(&promote_right), SOUTH_EAST);
	}

	bitboard_t	capture_left = shift_down_left(pawns_not_on_rank_7) & enemies;
	bitboard_t	capture_right = shift_down_right(pawns_not_on_rank_7) & enemies;

	while (capture_left)
	{
		square_t	to = pop_first_square(&capture_left);
		(movelist++)->move = create_move(to - SOUTH_WEST, to);
	}

	while (capture_right)
	{
		square_t	to = pop_first_square(&capture_right);
		(movelist++)->move = create_move(to - SOUTH_EAST, to);
	}

	if (board->stack->en_passant_square != SQ_NONE)
	{
		bitboard_t	capture_en_passant = pawns_not_on_rank_7
			& pawn_moves(board->stack->en_passant_square, WHITE);

		while (capture_en_passant)
			(movelist++)->move = create_en_passant(
				pop_first_square(&capture_en_passant),
				board->stack->en_passant_square);
	}

	return (movelist);
}

extmove_t	*generate_white_classic(extmove_t *movelist, const board_t *board,
			bitboard_t target)
{
	movelist = generate_classic_white_pawn_moves(movelist, board);
	movelist = generate_knight_moves(movelist, board, WHITE, target);
	movelist = generate_bishop_moves(movelist, board, WHITE, target);
	movelist = generate_rook_moves(movelist, board, WHITE, target);
	movelist = generate_queen_moves(movelist, board, WHITE, target);

	square_t	king_square = board_king_square(board, WHITE);
	bitboard_t	b = king_moves(king_square) & target;

	while (b)
		(movelist++)->move = create_move(king_square, pop_first_square(&b));

	if (!castling_blocked(board, WHITE_OO)
		&& (board->stack->castlings & WHITE_OO))
	{
		(movelist++)->move = create_castling(king_square,
			board->castling_rook_square[WHITE_OO]);
	}

	if (!castling_blocked(board, WHITE_OOO)
		&& (board->stack->castlings & WHITE_OOO))
	{
		(movelist++)->move = create_castling(king_square,
			board->castling_rook_square[WHITE_OOO]);
	}

	return (movelist);
}

extmove_t	*generate_black_classic(extmove_t *movelist, const board_t *board,
			bitboard_t target)
{
	movelist = generate_classic_black_pawn_moves(movelist, board);
	movelist = generate_knight_moves(movelist, board, BLACK, target);
	movelist = generate_bishop_moves(movelist, board, BLACK, target);
	movelist = generate_rook_moves(movelist, board, BLACK, target);
	movelist = generate_queen_moves(movelist, board, BLACK, target);

	square_t	king_square = board_king_square(board, BLACK);
	bitboard_t	b = king_moves(king_square) & target;

	while (b)
		(movelist++)->move = create_move(king_square, pop_first_square(&b));

	if (!castling_blocked(board, BLACK_OO)
		&& (board->stack->castlings & BLACK_OO))
	{
		(movelist++)->move = create_castling(king_square,
			board->castling_rook_square[BLACK_OO]);
	}

	if (!castling_blocked(board, BLACK_OOO)
		&& (board->stack->castlings & BLACK_OOO))
	{
		(movelist++)->move = create_castling(king_square,
			board->castling_rook_square[BLACK_OOO]);
	}

	return (movelist);
}

extmove_t	*generate_classic(extmove_t *movelist, const board_t *board)
{
	color_t		us = board->side_to_move;
	bitboard_t	target = ~board->color_bits[us];

	return (us == WHITE ? generate_white_classic(movelist, board, target)
		: generate_black_classic(movelist, board, target));
}
