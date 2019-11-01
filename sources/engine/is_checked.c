/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   is_checked.c                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 01:31:51 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/01 13:02:52 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <string.h>
#include <stdlib.h>

int		is_checked(board_t *board)
{
	int8_t	kingpos;

	{
		int8_t	*kp = memchr(board->table, (board->player == PLAYER_WHITE) ?
			BLACK_KING : WHITE_KING, 64);

		if (kp == NULL)
			return (1);

		kingpos = (int8_t)(kp - board->table);
	}

	int8_t	kfile = (kingpos & 7);
	int8_t	krank = (kingpos >> 3);
	int8_t	delta;
	int8_t	sqi;

	const int8_t pw_side_attack = (board->player == PLAYER_WHITE) ? NORTH : SOUTH;

	for (int8_t square = SQ_A1; square <= SQ_H8; square++)
	{
		int8_t	file = (square & 7);
		int8_t	rank = (square >> 3);

		switch (board->table[square] ^ (board->player << 3))
		{
			case WHITE_PAWN:
				if (file > FILE_A)
					if (square + pw_side_attack + WEST == kingpos)
						return (1);
				if (file < FILE_H)
					if (square + pw_side_attack + EAST == kingpos)
						return (1);
				break ;

			case WHITE_KNIGHT:
				if (file > FILE_A)
				{
					if (rank > RANK_2)
						if (square + SOUTH * 2 + WEST == kingpos)
							return (1);
					if (rank < RANK_7)
						if (square + NORTH * 2 + WEST == kingpos)
							return (1);
				}
				if (file > FILE_B)
				{
					if (rank > RANK_1)
						if (square + SOUTH + WEST * 2 == kingpos)
							return (1);
					if (rank < RANK_8)
						if (square + NORTH + WEST * 2 == kingpos)
							return (1);
				}
				if (file < FILE_H)
				{
					if (rank > RANK_2)
						if (square + SOUTH * 2 + EAST == kingpos)
							return (1);
					if (rank < RANK_7)
						if (square + NORTH * 2 + EAST == kingpos)
							return (1);
				}
				if (file < FILE_G)
				{
					if (rank > RANK_1)
						if (square + SOUTH + EAST * 2 == kingpos)
							return (1);
					if (rank < RANK_8)
						if (square + NORTH + EAST * 2 == kingpos)
							return (1);
				}
				break ;

			case WHITE_BISHOP:
				if (file - kfile != rank - krank && file - kfile != krank - rank)
					break ;
				if (file < kfile)
					delta = (rank < krank) ? NORTH_EAST : SOUTH_EAST;
				else
					delta = (rank < krank) ? NORTH_WEST : SOUTH_WEST;

				sqi = square;

				do
				{
					sqi += delta;
					if (sqi == kingpos)
						return (1);
				}
				while (board->table[sqi] == PIECE_NONE);
				break ;

			case WHITE_ROOK:
				if (file != kfile && rank != krank)
					break ;
				if (file == kfile)
					delta = (rank < krank) ? NORTH : SOUTH;
				else
					delta = (file < kfile) ? EAST : WEST;

				sqi = square;

				do
				{
					sqi += delta;
					if (sqi == kingpos)
						return (1);
				}
				while (board->table[sqi] == PIECE_NONE);
				break ;

			case WHITE_QUEEN:
				if (file != kfile && rank != krank
					&& file - kfile != rank - krank && file - kfile != krank - rank)
					break ;
				if (file == kfile)
					delta = (rank < krank) ? NORTH : SOUTH;
				else if (rank == krank)
					delta = (file < kfile) ? EAST : WEST;
				else if (file < kfile)
					delta = (rank < krank) ? NORTH_EAST : SOUTH_EAST;
				else
					delta = (rank < krank) ? NORTH_WEST : SOUTH_WEST;

				sqi = square;

				do
				{
					sqi += delta;
					if (sqi == kingpos)
						return (1);
				}
				while (board->table[sqi] == PIECE_NONE);
				break ;

			case WHITE_KING:
				if (abs(file - kfile) <= 1 && abs(rank - krank) <= 1)
					return (1);
				break ;
		}
	}
	return (0);
}
