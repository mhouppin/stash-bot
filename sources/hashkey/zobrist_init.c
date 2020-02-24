/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   zobrist_init.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 18:18:59 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 19:58:38 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

# include <math.h>
# include "bitboard.h"
# include "hashkey.h"

hashkey_t	ZobristPsq[PIECE_NB][SQUARE_NB];
hashkey_t	ZobristEnPassant[FILE_NB];
hashkey_t	ZobristCastling[CASTLING_NB];
hashkey_t	ZobristBlackToMove;

hashkey_t	microhash(int cr)
{
	return (cr * 0x7F6E5D4C3B2A1908ull + cr * cr * 0x37BF26AE159D048Cull + 1);
}

void	zobrist_init(void)
{
	for (piece_t piece = WHITE_PAWN; piece <= BLACK_KING; ++piece)
	{
		// Weird hash, but it works quite nicely

		hashkey_t	seed = (piece + 16);

		seed = (seed * seed);
		seed = (seed * seed);
		seed = (seed * seed);
		seed %= UINT32_MAX;
		seed *= seed;
		seed %= UINT32_MAX;
		seed *= seed;

		for (square_t square = SQ_A1; square <= SQ_H8; ++square)
		{
			seed *= 31;
			ZobristPsq[piece][square] = seed;
		}
	}

	for (file_t file = FILE_A; file <= FILE_H; ++file)
	{
		// I really don't know what I'm doing here

		union { float f[2]; hashkey_t k; } value = {{sinf(file + 1), cosf(file + 1)}};
		ZobristEnPassant[file] = value.k * value.k;
	}

	for (int cr = 0; cr < CASTLING_NB; ++cr)
	{
		ZobristCastling[cr] = 0;
		bitboard_t	b = cr;
		while (b)
		{
			hashkey_t	k = ZobristCastling[1ull << pop_first_square(&b)];
			ZobristCastling[cr] ^= k ? k : microhash(cr);
		}
	}

	ZobristBlackToMove = ~(bitboard_t)0;
}
