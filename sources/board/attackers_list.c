/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   attackers_list.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 15:37:49 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:24:12 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "board.h"

bitboard_t		attackers_list(const board_t *board, square_t s,
				bitboard_t occupied)
{
	return ((pawn_moves(s, BLACK) & board->piecetype_bits[PAWN]
		& board->color_bits[WHITE])
		| (pawn_moves(s, WHITE) & board->piecetype_bits[PAWN]
		& board->color_bits[BLACK])
		| (knight_moves(s) & board->piecetype_bits[KNIGHT])
		| (rook_move_bits(s, occupied) & board_pieces(board, ROOK, QUEEN))
		| (bishop_move_bits(s, occupied) & board_pieces(board, BISHOP, QUEEN))
		| (king_moves(s) & board->piecetype_bits[KING]));
}
