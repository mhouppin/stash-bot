/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   is_checked.c                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 01:31:51 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/23 14:05:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <string.h>
#include <stdlib.h>

int		is_checked(const board_t *board)
{

	int8_t	kingpos;

	{
		int8_t	*kp = memchr(board->table, (board->player == PLAYER_WHITE) ?
			BLACK_KING : WHITE_KING, 64);

		if (kp == NULL)
			return (1);

		kingpos = (int8_t)(kp - board->table);
	}

	const int8_t	kfile = (kingpos & 7);
	const int8_t	krank = (kingpos >> 3);
	const int8_t	plxor = board->player << 3;

	// Check for pawn attacks

	{
		int8_t pw_attack_from = (board->player == PLAYER_WHITE) ? SOUTH : NORTH;

		if (kingpos + pw_attack_from >= SQ_A1 && kingpos + pw_attack_from <= SQ_H8)
		{
			if (kfile > FILE_A)
				if ((board->table[kingpos + pw_attack_from + WEST] ^ plxor) == WHITE_PAWN)
					return (1);
			if (kfile < FILE_H)
				if ((board->table[kingpos + pw_attack_from + EAST] ^ plxor) == WHITE_PAWN)
					return (1);
		}
	}

	// Check for knight attacks

	{
		if (kfile > FILE_A)
		{
			if (krank > RANK_2)
				if ((board->table[kingpos + SOUTH * 2 + WEST] ^ plxor) == WHITE_KNIGHT)
					return (1);
			if (krank < RANK_7)
				if ((board->table[kingpos + NORTH * 2 + WEST] ^ plxor) == WHITE_KNIGHT)
					return (1);

			if (kfile > FILE_B)
			{
				if (krank > RANK_1)
					if ((board->table[kingpos + SOUTH + WEST * 2] ^ plxor) == WHITE_KNIGHT)
						return (1);

				if (krank < RANK_8)
					if ((board->table[kingpos + NORTH + WEST * 2] ^ plxor) == WHITE_KNIGHT)
						return (1);
			}
		}
		if (kfile < FILE_H)
		{
			if (krank > RANK_2)
				if ((board->table[kingpos + SOUTH * 2 + EAST] ^ plxor) == WHITE_KNIGHT)
					return (1);
			if (krank < RANK_7)
				if ((board->table[kingpos + NORTH * 2 + EAST] ^ plxor) == WHITE_KNIGHT)
					return (1);

			if (kfile < FILE_G)
			{
				if (krank > RANK_1)
					if ((board->table[kingpos + SOUTH + EAST * 2] ^ plxor) == WHITE_KNIGHT)
						return (1);

				if (krank < RANK_8)
					if ((board->table[kingpos + NORTH + EAST * 2] ^ plxor) == WHITE_KNIGHT)
						return (1);
			}
		}
	}

	// Check for linear attacks (rook/queen)

	{
		int8_t next = kingpos;

		while ((next & 7) > FILE_A)
		{
			next += WEST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_ROOK || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}

		next = kingpos;

		while ((next & 7) < FILE_H)
		{
			next += EAST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_ROOK || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}

		next = kingpos;

		while (next < SQ_A8)
		{
			next += NORTH;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_ROOK || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}

		next = kingpos;

		while (next > SQ_H1)
		{
			next += SOUTH;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_ROOK || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}
	}

	// Check for diagonal attacks (bishop/queen)

	{
		int8_t	next = kingpos;

		while ((next & 7) > FILE_A && next < SQ_A8)
		{
			next += NORTH_WEST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_BISHOP || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}
		
		next = kingpos;

		while ((next & 7) < FILE_H && next < SQ_A8)
		{
			next += NORTH_EAST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_BISHOP || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}
		
		next = kingpos;

		while ((next & 7) > FILE_A && next > SQ_H1)
		{
			next += SOUTH_WEST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_BISHOP || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}
		
		next = kingpos;

		while ((next & 7) < FILE_H && next > SQ_H1)
		{
			next += SOUTH_EAST;
			int8_t piece = board->table[next];
			if ((piece ^ plxor) == WHITE_BISHOP || (piece ^ plxor) == WHITE_QUEEN)
				return (1);
			else if (piece)
				break ;
		}
	}

	return (0);
}
