/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   do_move.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 21:26:33 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/01 11:44:29 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

void	do_move(board_t *board, move_t move)
{
	const int16_t	from = move_from(move);
	const int16_t	to = move_to(move);
	int8_t			start_piece;

	if (from == SQ_E1 && board->table[from] == WHITE_KING)
	{
		board->special_moves &= ~(WHITE_CASTLING | 240);
		if (to == SQ_G1)
		{
			board->table[SQ_E1] = PIECE_NONE;
			board->table[SQ_F1] = WHITE_ROOK;
			board->table[SQ_G1] = WHITE_KING;
			board->table[SQ_H1] = PIECE_NONE;
			board->player = !(board->player);
		}
		else if (to == SQ_C1)
		{
			board->table[SQ_E1] = PIECE_NONE;
			board->table[SQ_D1] = WHITE_ROOK;
			board->table[SQ_C1] = WHITE_KING;
			board->table[SQ_A1] = PIECE_NONE;
			board->player = !(board->player);
		}
		else
			goto __standard;
	}
	else if (from == SQ_E8 && board->table[from] == BLACK_KING)
	{
		board->special_moves &= ~(BLACK_CASTLING | 240);
		if (to == SQ_G8)
		{
			board->table[SQ_E8] = PIECE_NONE;
			board->table[SQ_F8] = BLACK_ROOK;
			board->table[SQ_G8] = BLACK_KING;
			board->table[SQ_H8] = PIECE_NONE;
			board->player = !(board->player);
		}
		else if (to == SQ_C8)
		{
			board->table[SQ_E8] = PIECE_NONE;
			board->table[SQ_D8] = BLACK_ROOK;
			board->table[SQ_C8] = BLACK_KING;
			board->table[SQ_A8] = PIECE_NONE;
			board->player = !(board->player);
		}
		else
			goto __standard;
	}
	else
	{

__standard:
		start_piece = board->table[from];

		if ((move & SPE_MASK) == PROMOTION)
		{
			int player = board->player;

			if ((move & PROMOTION_MASK) == TO_KNIGHT)
				start_piece = (player == PLAYER_WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
			else if ((move & PROMOTION_MASK) == TO_BISHOP)
				start_piece = (player == PLAYER_WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
			else if ((move & PROMOTION_MASK) == TO_ROOK)
				start_piece = (player == PLAYER_WHITE) ? WHITE_ROOK : BLACK_ROOK;
			else
				start_piece = (player == PLAYER_WHITE) ? WHITE_QUEEN : BLACK_QUEEN;
		}
		else if (board->special_moves & EN_PASSANT_OK)
		{
			if (start_piece == WHITE_PAWN)
			{
				if (to == SQ_A6 + ((board->special_moves >> 4) & 7))
					board->table[to + SOUTH] = PIECE_NONE;
			}
			else if (start_piece == BLACK_PAWN)
			{
				if (to == SQ_A3 + ((board->special_moves >> 4) & 7))
					board->table[to + NORTH] = PIECE_NONE;
			}
		}
		if (start_piece == WHITE_ROOK)
		{
			if (from == SQ_A1)
				board->special_moves &= ~(WHITE_OOO);
			else if (from == SQ_H1)
				board->special_moves &= ~(WHITE_OO);
		}
		else if (start_piece == BLACK_ROOK)
		{
			if (from == SQ_A8)
				board->special_moves &= ~(BLACK_OOO);
			else
				board->special_moves &= ~(BLACK_OO);
		}
		else if ((start_piece == WHITE_PAWN && from + NORTH + NORTH == to)
			|| (start_piece == BLACK_PAWN && from + SOUTH + SOUTH == to))
		{
			board->special_moves &= ~(FILE_H << 4);
			board->special_moves |= EN_PASSANT_OK | ((from & 7) << 4);
		}
		else
			board->special_moves &= ~(240);

		if (board->table[to] == BLACK_ROOK)
		{
			if (to == SQ_A8)
				board->special_moves &= ~(BLACK_OOO);
			else if (to == SQ_H8)
				board->special_moves &= ~(BLACK_OO);
		}
		else if (board->table[to] == WHITE_ROOK)
		{
			if (to == SQ_A1)
				board->special_moves &= ~(WHITE_OOO);
			else if (to == SQ_H1)
				board->special_moves &= ~(WHITE_OO);
		}

		board->table[from] = PIECE_NONE;
		board->table[to] = start_piece;
		board->player = !(board->player);
	}
}
