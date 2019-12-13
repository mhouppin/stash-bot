/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   do_move.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 21:26:33 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/12/13 15:22:55 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

extern const int16_t	mpiece_score[8];
extern const int16_t	epiece_score[8];
extern const int16_t	mtable_score[8][64];
extern const int16_t	etable_score[8][64];

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

			board->mscore -= mtable_score[WHITE_KING][SQ_E1];
			board->escore -= etable_score[WHITE_KING][SQ_E1];
			board->mscore -= mtable_score[WHITE_ROOK][SQ_H1];
			board->escore -= etable_score[WHITE_ROOK][SQ_H1];
			board->mscore += mtable_score[WHITE_KING][SQ_G1];
			board->escore += etable_score[WHITE_KING][SQ_G1];
			board->mscore += mtable_score[WHITE_ROOK][SQ_F1];
			board->escore += etable_score[WHITE_ROOK][SQ_F1];

			board->player = !(board->player);
		}
		else if (to == SQ_C1)
		{
			board->table[SQ_E1] = PIECE_NONE;
			board->table[SQ_D1] = WHITE_ROOK;
			board->table[SQ_C1] = WHITE_KING;
			board->table[SQ_A1] = PIECE_NONE;

			board->mscore -= mtable_score[WHITE_KING][SQ_E1];
			board->escore -= etable_score[WHITE_KING][SQ_E1];
			board->mscore -= mtable_score[WHITE_ROOK][SQ_A1];
			board->escore -= etable_score[WHITE_ROOK][SQ_A1];
			board->mscore += mtable_score[WHITE_KING][SQ_C1];
			board->escore += etable_score[WHITE_KING][SQ_C1];
			board->mscore += mtable_score[WHITE_ROOK][SQ_D1];
			board->escore += etable_score[WHITE_ROOK][SQ_D1];

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

			board->mscore += mtable_score[WHITE_KING][SQ_E1];
			board->escore += etable_score[WHITE_KING][SQ_E1];
			board->mscore += mtable_score[WHITE_ROOK][SQ_H1];
			board->escore += etable_score[WHITE_ROOK][SQ_H1];
			board->mscore -= mtable_score[WHITE_KING][SQ_G1];
			board->escore -= etable_score[WHITE_KING][SQ_G1];
			board->mscore -= mtable_score[WHITE_ROOK][SQ_F1];
			board->escore -= etable_score[WHITE_ROOK][SQ_F1];

			board->player = !(board->player);
		}
		else if (to == SQ_C8)
		{
			board->table[SQ_E8] = PIECE_NONE;
			board->table[SQ_D8] = BLACK_ROOK;
			board->table[SQ_C8] = BLACK_KING;
			board->table[SQ_A8] = PIECE_NONE;

			board->mscore += mtable_score[WHITE_KING][SQ_E1];
			board->escore += etable_score[WHITE_KING][SQ_E1];
			board->mscore += mtable_score[WHITE_ROOK][SQ_A1];
			board->escore += etable_score[WHITE_ROOK][SQ_A1];
			board->mscore -= mtable_score[WHITE_KING][SQ_C1];
			board->escore -= etable_score[WHITE_KING][SQ_C1];
			board->mscore -= mtable_score[WHITE_ROOK][SQ_D1];
			board->escore -= etable_score[WHITE_ROOK][SQ_D1];

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

			if (player == PLAYER_WHITE)
			{
				board->mscore -= 100;
				board->escore -= 200;
				board->mscore -= mtable_score[WHITE_PAWN][from];
				board->escore -= etable_score[WHITE_PAWN][from];
				board->mscore += mpiece_score[start_piece];
				board->escore += epiece_score[start_piece];
				board->mscore += mtable_score[start_piece][to];
				board->escore += etable_score[start_piece][to];
			}
			else
			{
				board->mscore += 100;
				board->escore += 200;
				board->mscore += mtable_score[WHITE_PAWN][from ^ SQ_A8];
				board->escore += etable_score[WHITE_PAWN][from ^ SQ_A8];
				board->mscore -= mpiece_score[start_piece & 7];
				board->escore -= epiece_score[start_piece & 7];
				board->mscore -= mtable_score[start_piece & 7][to ^ SQ_A8];
				board->escore -= etable_score[start_piece & 7][to ^ SQ_A8];
			}
		}
		else if (board->special_moves & EN_PASSANT_OK)
		{
			if (start_piece == WHITE_PAWN)
			{
				if (to == SQ_A6 + ((board->special_moves >> 4) & 7))
				{
					board->table[to + SOUTH] = PIECE_NONE;
					board->mscore += mtable_score[WHITE_PAWN][(to + SOUTH) ^ SQ_A8];
					board->escore += etable_score[WHITE_PAWN][(to + SOUTH) ^ SQ_A8];
					board->mscore += 100;
					board->escore += 200;
					board->pcount--;
				}
			}
			else if (start_piece == BLACK_PAWN)
			{
				if (to == SQ_A3 + ((board->special_moves >> 4) & 7))
				{
					board->table[to + NORTH] = PIECE_NONE;
					board->mscore -= mtable_score[WHITE_PAWN][to + NORTH];
					board->escore -= etable_score[WHITE_PAWN][to + NORTH];
					board->mscore -= 100;
					board->escore -= 200;
					board->pcount--;
				}
			}
		}
		board->special_moves &= ~(EN_PASSANT_OK);
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
		
		const int8_t	end_piece = board->table[to];

		board->table[from] = PIECE_NONE;
		board->table[to] = start_piece;
		if ((move & SPE_MASK) != PROMOTION)
		{
			if (board->player == PLAYER_WHITE)
			{
				board->mscore -= mtable_score[start_piece][from];
				board->escore -= etable_score[start_piece][from];
				board->mscore += mtable_score[start_piece][to];
				board->escore += etable_score[start_piece][to];
				if (end_piece != PIECE_NONE)
				{
					board->mscore += mtable_score[end_piece & 7][to ^ SQ_A8];
					board->escore += etable_score[end_piece & 7][to ^ SQ_A8];
					board->mscore += mpiece_score[end_piece & 7];
					board->escore += epiece_score[end_piece & 7];
					board->pcount--;
				}
			}
			else
			{
				board->mscore += mtable_score[start_piece & 7][from ^ SQ_A8];
				board->escore += etable_score[start_piece & 7][from ^ SQ_A8];
				board->mscore -= mtable_score[start_piece & 7][to ^ SQ_A8];
				board->escore -= etable_score[start_piece & 7][to ^ SQ_A8];
				if (end_piece != PIECE_NONE)
				{
					board->mscore -= mtable_score[end_piece][to];
					board->escore -= etable_score[end_piece][to];
					board->mscore -= mpiece_score[end_piece];
					board->escore -= epiece_score[end_piece];
					board->pcount--;
				}
			}
		}
		board->player = !(board->player);
	}
}
