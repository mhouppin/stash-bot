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

extmove_t	*create_evasion_promotions(extmove_t *movelist, square_t to,
			direction_t direction)
{
	(movelist++)->move = create_promotion(to - direction, to, QUEEN);
	(movelist++)->move = create_promotion(to - direction, to, ROOK);
	(movelist++)->move = create_promotion(to - direction, to, BISHOP);
	(movelist++)->move = create_promotion(to - direction, to, KNIGHT);

	return (movelist);
}

extmove_t	*generate_white_pawn_evasion_moves(extmove_t *movelist,
			const board_t *board, bitboard_t block_squares)
{
	bitboard_t		empty_squares;
	bitboard_t		pawns_on_rank_7 = board->piecetype_bits[PAWN]
		& board->color_bits[WHITE] & RANK_7_BITS;

	bitboard_t		pawns_not_on_rank_7 = board->piecetype_bits[PAWN]
		& board->color_bits[WHITE] & ~RANK_7_BITS;

	bitboard_t		enemies = board->color_bits[BLACK] & block_squares;

	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	bitboard_t		push = shift_up(pawns_not_on_rank_7) & empty_squares;
	bitboard_t		push2 = shift_up(push & RANK_3_BITS) & empty_squares;

	push &= block_squares;
	push2 &= block_squares;

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
		empty_squares &= block_squares;

		bitboard_t	promote = shift_up(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_up_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_up_right(pawns_on_rank_7) & enemies;

		while (promote)
			movelist = create_evasion_promotions(movelist,
				pop_first_square(&promote), NORTH);

		while (promote_left)
			movelist = create_evasion_promotions(movelist,
				pop_first_square(&promote_left), NORTH_WEST);

		while (promote_right)
			movelist = create_evasion_promotions(movelist,
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
		if (!(block_squares & square_bit(board->stack->en_passant_square - NORTH)))
			return (movelist);

		bitboard_t	capture_en_passant = pawns_not_on_rank_7
			& pawn_moves(board->stack->en_passant_square, BLACK);

		while (capture_en_passant)
			(movelist++)->move = create_en_passant(
				pop_first_square(&capture_en_passant),
				board->stack->en_passant_square);
	}

	return (movelist);
}

extmove_t	*generate_black_pawn_evasion_moves(extmove_t *movelist,
			const board_t *board, bitboard_t block_squares)
{
	bitboard_t		empty_squares;
	bitboard_t		pawns_on_rank_7 = board->piecetype_bits[PAWN]
		& board->color_bits[BLACK] & RANK_2_BITS;

	bitboard_t		pawns_not_on_rank_7 = board->piecetype_bits[PAWN]
		& board->color_bits[BLACK] & ~RANK_2_BITS;

	bitboard_t		enemies = board->color_bits[WHITE] & block_squares;

	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	bitboard_t		push = shift_down(pawns_not_on_rank_7) & empty_squares;
	bitboard_t		push2 = shift_down(push & RANK_6_BITS) & empty_squares;

	push &= block_squares;
	push2 &= block_squares;

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
		empty_squares &= block_squares;

		bitboard_t	promote = shift_down(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_down_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_down_right(pawns_on_rank_7) & enemies;

		while (promote)
			movelist = create_evasion_promotions(movelist,
				pop_first_square(&promote), SOUTH);

		while (promote_left)
			movelist = create_evasion_promotions(movelist,
				pop_first_square(&promote_left), SOUTH_WEST);

		while (promote_right)
			movelist = create_evasion_promotions(movelist,
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
		if (!(block_squares & square_bit(board->stack->en_passant_square - SOUTH)))
			return (movelist);

		bitboard_t	capture_en_passant = pawns_not_on_rank_7
			& pawn_moves(board->stack->en_passant_square, WHITE);

		while (capture_en_passant)
			(movelist++)->move = create_en_passant(
				pop_first_square(&capture_en_passant),
				board->stack->en_passant_square);
	}

	return (movelist);

}

extmove_t	*generate_white_evasions(extmove_t *movelist, const board_t *board,
			bitboard_t block_squares)
{
	movelist = generate_white_pawn_evasion_moves(movelist, board, block_squares);
	movelist = generate_knight_moves(movelist, board, WHITE, block_squares);
	movelist = generate_bishop_moves(movelist, board, WHITE, block_squares);
	movelist = generate_rook_moves(movelist, board, WHITE, block_squares);
	movelist = generate_queen_moves(movelist, board, WHITE, block_squares);

	return (movelist);
}

extmove_t	*generate_black_evasions(extmove_t *movelist, const board_t *board,
			bitboard_t block_squares)
{
	movelist = generate_black_pawn_evasion_moves(movelist, board, block_squares);
	movelist = generate_knight_moves(movelist, board, BLACK, block_squares);
	movelist = generate_bishop_moves(movelist, board, BLACK, block_squares);
	movelist = generate_rook_moves(movelist, board, BLACK, block_squares);
	movelist = generate_queen_moves(movelist, board, BLACK, block_squares);

	return (movelist);
}

extmove_t	*generate_evasions(extmove_t *movelist, const board_t *board)
{
	color_t		us = board->side_to_move;
	square_t	king_square = board->piece_list[create_piece(us, KING)][0];
	bitboard_t	slider_attacks = 0;
	bitboard_t	sliders = board->stack->checkers
		& ~board_pieces(board, KNIGHT, PAWN);

	while (sliders)
	{
		square_t	check_square = pop_first_square(&sliders);
		slider_attacks |= LineBits[check_square][king_square]
			^ square_bit(check_square);
	}

	bitboard_t	b = king_moves(king_square) & ~board->color_bits[us]
		& ~slider_attacks;

	while (b)
		(movelist++)->move = create_move(king_square, pop_first_square(&b));

	if (more_than_one(board->stack->checkers))
		return (movelist);

	square_t	check_square = first_square(board->stack->checkers);
	bitboard_t	target = squares_between(check_square, king_square)
		| square_bit(check_square);

	if (us == WHITE)
		return (generate_white_evasions(movelist, board, target));
	else
		return (generate_black_evasions(movelist, board, target));
}
