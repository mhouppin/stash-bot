/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   get_piece_moves.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 02:17:22 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 05:58:29 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

inline int	empty(board_t *board, int8_t square)
{
	return (board->table[square] == PIECE_NONE);
}

inline int	opponent(board_t *board, int8_t square)
{
	return ((board->table[square] ^ (board->player << 3)) >= BLACK_PAWN);
}

void		get_pawn_moves(movelist_t *mlist, int8_t sq, board_t *board)
{
	const int8_t	pw_side_attack =
		(board->player == PLAYER_WHITE ? NORTH : SOUTH);
	const int8_t	file = (sq & 7);
	const int8_t	rank = (sq >> 3);

	if (empty(board, sq + pw_side_attack))
	{
		move_t	move = get_move(sq, sq + pw_side_attack);
		if (rank == (board->player == PLAYER_WHITE ? RANK_7 : RANK_2))
		{
			push_move(mlist, move | PROMOTION | TO_KNIGHT);
			push_move(mlist, move | PROMOTION | TO_BISHOP);
			push_move(mlist, move | PROMOTION | TO_ROOK);
			push_move(mlist, move | PROMOTION | TO_QUEEN);
		}
		else
		{
			push_move(mlist, move);
			if (rank == (board->player == PLAYER_WHITE ? RANK_2 : RANK_7))
				if (empty(board, sq + pw_side_attack * 2))
					push_move(mlist, get_move(sq, sq + pw_side_attack * 2));
		}
	}

	if (file != FILE_A)
	{
		if (opponent(board, sq + pw_side_attack + WEST))
			push_move(mlist, get_move(sq, sq + pw_side_attack + WEST));
		else if (rank == (board->player == PLAYER_WHITE ? RANK_5 : RANK_4)
				&& (board->special_moves & EN_PASSANT_OK)
				&& ((board->special_moves >> 4) & 7) == file - 1)
		{
			push_move(mlist, get_move(sq, sq + pw_side_attack + WEST));
		}
	}

	if (file != FILE_H)
	{
		if (opponent(board, sq + pw_side_attack + EAST))
			push_move(mlist, get_move(sq, sq + pw_side_attack + EAST));
		else if (rank == (board->player == PLAYER_WHITE ? RANK_5 : RANK_4)
				&& (board->special_moves & EN_PASSANT_OK)
				&& ((board->special_moves >> 4) & 7) == file + 1)
		{
			push_move(mlist, get_move(sq, sq + pw_side_attack + EAST));
		}
	}
}

void		get_knight_moves(movelist_t *mlist, int8_t sq, board_t *board)
{
	const int8_t	file = (sq & 7);
	const int8_t	rank = (sq >> 3);

	if (file > FILE_A)
	{
		if (rank > RANK_2)
			if (empty(board, sq - 17) || opponent(board, sq - 17))
				push_move(mlist, get_move(sq, sq - 17));
		if (rank < RANK_7)
			if (empty(board, sq + 15) || opponent(board, sq + 15))
				push_move(mlist, get_move(sq, sq + 15));
		if (file > FILE_B)
		{
			if (rank > RANK_1)
				if (empty(board, sq - 10) || opponent(board, sq - 10))
					push_move(mlist, get_move(sq, sq - 10));
			if (rank < RANK_8)
				if (empty(board, sq + 6) || opponent(board, sq + 6))
					push_move(mlist, get_move(sq, sq + 6));
		}
	}
	if (file < FILE_H)
	{
		if (rank > RANK_2)
			if (empty(board, sq - 15) || opponent(board, sq - 15))
				push_move(mlist, get_move(sq, sq - 15));
		if (rank < RANK_7)
			if (empty(board, sq + 17) || opponent(board, sq + 17))
				push_move(mlist, get_move(sq, sq + 17));
		if (file < FILE_G)
		{
			if (rank > RANK_1)
				if (empty(board, sq - 6) || opponent(board, sq - 6))
					push_move(mlist, get_move(sq, sq - 6));
			if (rank < RANK_8)
				if (empty(board, sq + 10) || opponent(board, sq + 10))
					push_move(mlist, get_move(sq, sq + 10));
		}
	}
}

void		get_bishop_moves(movelist_t *mlist, int8_t sq, board_t *board)
{
	const int8_t	bottom = (sq >> 3);
	const int8_t	top = (7 - bottom);
	const int8_t	left = (sq & 7);
	const int8_t	right = (7 - left);

	const int8_t	tl_min = (top < left) ? top : left;
	const int8_t	tr_min = (top < right) ? top : right;
	const int8_t	bl_min = (bottom < left) ? bottom : left;
	const int8_t	br_min = (bottom < right) ? bottom : right;

	int8_t	delta = 0;
	for (int8_t i = 0; i < tl_min; i++)
	{
		delta += NORTH_WEST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < tr_min; i++)
	{
		delta += NORTH_EAST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < bl_min; i++)
	{
		delta += SOUTH_WEST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < br_min; i++)
	{
		delta += SOUTH_EAST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}
}

void		get_rook_moves(movelist_t *mlist, int8_t sq, board_t *board)
{
	const int8_t	bottom = (sq >> 3);
	const int8_t	top = (7 - bottom);
	const int8_t	left = (sq & 7);
	const int8_t	right = (7 - left);

	int8_t	delta = 0;
	for (int8_t i = 0; i < top; i++)
	{
		delta += NORTH;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < bottom; i++)
	{
		delta += SOUTH;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < left; i++)
	{
		delta += WEST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}

	delta = 0;
	for (int8_t i = 0; i < right; i++)
	{
		delta += EAST;
		if (empty(board, sq + delta))
			push_move(mlist, get_move(sq, sq + delta));
		else
		{
			if (opponent(board, sq + delta))
				push_move(mlist, get_move(sq, sq + delta));
			break ;
		}
	}
}

void		get_king_moves(movelist_t *mlist, int8_t sq, board_t *board)
{
	const int8_t	file = (sq & 7);
	const int8_t	rank = (sq >> 3);

	if (rank != RANK_1)
	{
		if (file != FILE_A)
			if (empty(board, sq + SOUTH_WEST) || opponent(board, sq + SOUTH_WEST))
				push_move(mlist, get_move(sq, sq + SOUTH_WEST));
		if (empty(board, sq + SOUTH) || opponent(board, sq + SOUTH))
			push_move(mlist, get_move(sq, sq + SOUTH));
		if (file != FILE_H)
			if (empty(board, sq + SOUTH_EAST) || opponent(board, sq + SOUTH_EAST))
				push_move(mlist, get_move(sq, sq + SOUTH_EAST));
	}
	if (file != FILE_A)
		if (empty(board, sq + WEST) || opponent(board, sq + WEST))
			push_move(mlist, get_move(sq, sq + WEST));
	if (file != FILE_H)
		if (empty(board, sq + EAST) || opponent(board, sq + EAST))
			push_move(mlist, get_move(sq, sq + EAST));
	if (rank != RANK_8)
	{
		if (file != FILE_A)
			if (empty(board, sq + NORTH_WEST) || opponent(board, sq + NORTH_WEST))
				push_move(mlist, get_move(sq, sq + NORTH_WEST));
		if (empty(board, sq + NORTH) || opponent(board, sq + NORTH))
			push_move(mlist, get_move(sq, sq + NORTH));
		if (file != FILE_H)
			if (empty(board, sq + NORTH_EAST) || opponent(board, sq + NORTH_EAST))
				push_move(mlist, get_move(sq, sq + NORTH_EAST));
	}
}
