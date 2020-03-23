/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_captures.c                              .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/02 15:30:30 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/02 15:57:40 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

extmove_t	*generate_capture_white_pawn_moves(extmove_t *movelist,
			const board_t *board)
{
	bitboard_t	pawns_on_rank_7 = board->color_bits[WHITE]
		& board->piecetype_bits[PAWN] & RANK_7_BITS;
	bitboard_t	pawns_not_on_rank_7 = board->color_bits[WHITE]
		& board->piecetype_bits[PAWN] & ~RANK_7_BITS;

	bitboard_t	enemies = board->color_bits[BLACK];
	bitboard_t	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	if (pawns_on_rank_7)
	{
		bitboard_t	promote = shift_up(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_up_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_up_right(pawns_on_rank_7) & enemies;

		while (promote)
		{
			square_t	to = pop_first_square(&promote);
			(movelist++)->move = create_promotion(to - NORTH, to, QUEEN);
		}

		while (promote_left)
		{
			square_t	to = pop_first_square(&promote_left);
			(movelist++)->move = create_promotion(to - NORTH_WEST, to, QUEEN);
		}

		while (promote_right)
		{
			square_t	to = pop_first_square(&promote_right);
			(movelist++)->move = create_promotion(to - NORTH_EAST, to, QUEEN);
		}
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

extmove_t	*generate_capture_black_pawn_moves(extmove_t *movelist,
			const board_t *board)
{
	bitboard_t	pawns_on_rank_7 = board->color_bits[BLACK]
		& board->piecetype_bits[PAWN] & RANK_2_BITS;
	bitboard_t	pawns_not_on_rank_7 = board->color_bits[BLACK]
		& board->piecetype_bits[PAWN] & ~RANK_2_BITS;

	bitboard_t	enemies = board->color_bits[WHITE];
	bitboard_t	empty_squares = ~board->piecetype_bits[ALL_PIECES];

	if (pawns_on_rank_7)
	{
		bitboard_t	promote = shift_down(pawns_on_rank_7) & empty_squares;
		bitboard_t	promote_left = shift_down_left(pawns_on_rank_7) & enemies;
		bitboard_t	promote_right = shift_down_right(pawns_on_rank_7) & enemies;

		while (promote)
		{
			square_t	to = pop_first_square(&promote);
			(movelist++)->move = create_promotion(to - SOUTH, to, QUEEN);
		}

		while (promote_left)
		{
			square_t	to = pop_first_square(&promote_left);
			(movelist++)->move = create_promotion(to - SOUTH_WEST, to, QUEEN);
		}

		while (promote_right)
		{
			square_t	to = pop_first_square(&promote_right);
			(movelist++)->move = create_promotion(to - SOUTH_EAST, to, QUEEN);
		}
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

extmove_t	*generate_white_captures(extmove_t *movelist, const board_t *board,
			bitboard_t target)
{
	movelist = generate_capture_white_pawn_moves(movelist, board);
	movelist = generate_knight_moves(movelist, board, WHITE, target);
	movelist = generate_bishop_moves(movelist, board, WHITE, target);
	movelist = generate_rook_moves(movelist, board, WHITE, target);
	movelist = generate_queen_moves(movelist, board, WHITE, target);

	square_t	king_square = board->piece_list[WHITE_KING][0];
	bitboard_t	b = king_moves(king_square) & target;

	while (b)
		(movelist++)->move = create_move(king_square, pop_first_square(&b));

	return (movelist);
}

extmove_t	*generate_black_captures(extmove_t *movelist, const board_t *board,
			bitboard_t target)
{
	movelist = generate_capture_black_pawn_moves(movelist, board);
	movelist = generate_knight_moves(movelist, board, BLACK, target);
	movelist = generate_bishop_moves(movelist, board, BLACK, target);
	movelist = generate_rook_moves(movelist, board, BLACK, target);
	movelist = generate_queen_moves(movelist, board, BLACK, target);

	square_t	king_square = board->piece_list[BLACK_KING][0];
	bitboard_t	b = king_moves(king_square) & target;

	while (b)
		(movelist++)->move = create_move(king_square, pop_first_square(&b));

	return (movelist);
}

extmove_t	*generate_captures(extmove_t *movelist, const board_t *board)
{
	color_t		us = board->side_to_move;
	bitboard_t	target = board->color_bits[opposite_color(us)];

	return (us == WHITE ? generate_white_captures(movelist, board, target)
		: generate_black_captures(movelist, board, target));
}
