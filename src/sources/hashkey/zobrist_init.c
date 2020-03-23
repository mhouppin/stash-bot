/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   zobrist_init.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 18:18:59 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 12:06:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

# include <math.h>
# include "bitboard.h"
# include "hashkey.h"
# include "random.h"

hashkey_t	ZobristPsq[PIECE_NB][SQUARE_NB];
hashkey_t	ZobristEnPassant[FILE_NB];
hashkey_t	ZobristCastling[CASTLING_NB];
hashkey_t	ZobristBlackToMove;

void	zobrist_init(void)
{
	qseed(0x7F6E5D4C3B2A1908ull);

	for (piece_t piece = WHITE_PAWN; piece <= BLACK_KING; ++piece)
		for (square_t square = SQ_A1; square <= SQ_H8; ++square)
			ZobristPsq[piece][square] = qrandom();

	for (file_t file = FILE_A; file <= FILE_H; ++file)
		ZobristEnPassant[file] = qrandom();

	for (int cr = 0; cr < CASTLING_NB; ++cr)
	{
		ZobristCastling[cr] = 0;
		bitboard_t	b = cr;
		while (b)
		{
			hashkey_t	k = ZobristCastling[1ull << pop_first_square(&b)];
			ZobristCastling[cr] ^= k ? k : qrandom();
		}
	}

	ZobristBlackToMove = ~(hashkey_t)0;
}
