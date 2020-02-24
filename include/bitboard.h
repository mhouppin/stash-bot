/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   bitboard.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:10:31 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 08:14:35 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef BITBOARD_H
# define BITBOARD_H

# include <immintrin.h>
# include <stdbool.h>
# include <stdint.h>
# include "inlining.h"
# include "piece.h"
# include "square.h"

typedef uint64_t	bitboard_t;

# define FILE_A_BITS	0x0101010101010101ull
# define FILE_B_BITS	0x0202020202020202ull
# define FILE_C_BITS	0x0404040404040404ull
# define FILE_D_BITS	0x0808080808080808ull
# define FILE_E_BITS	0x1010101010101010ull
# define FILE_F_BITS	0x2020202020202020ull
# define FILE_G_BITS	0x4040404040404040ull
# define FILE_H_BITS	0x8080808080808080ull

# define RANK_1_BITS	0x00000000000000FFull
# define RANK_2_BITS	0x000000000000FF00ull
# define RANK_3_BITS	0x0000000000FF0000ull
# define RANK_4_BITS	0x00000000FF000000ull
# define RANK_5_BITS	0x000000FF00000000ull
# define RANK_6_BITS	0x0000FF0000000000ull
# define RANK_7_BITS	0x00FF000000000000ull
# define RANK_8_BITS	0xFF00000000000000ull

# define FULL_BITS		0xFFFFFFFFFFFFFFFFull

extern bitboard_t	LineBits[SQUARE_NB][SQUARE_NB];
extern bitboard_t	PseudoMoves[PIECETYPE_NB][SQUARE_NB];
extern bitboard_t	PawnMoves[COLOR_NB][SQUARE_NB];

typedef struct	magic_s
{
	bitboard_t		mask;
	bitboard_t		magic;
	bitboard_t		*moves;
	unsigned int	shift;
}				magic_t;

INLINED unsigned int	magic_index(const magic_t *magic, bitboard_t occupied)
{
# ifdef USE_PEXT
	return (_pext_u64(occupied, magic->mask));
# else
	return ((unsigned int)(((occupied & magic->mask) * magic->magic)
		>> magic->shift));
#endif
}

extern magic_t		RookMagics[SQUARE_NB];
extern magic_t		BishopMagics[SQUARE_NB];

INLINED bitboard_t	square_bit(square_t square)
{
	return (1ull << square);
}

INLINED bitboard_t	shift_up(bitboard_t b)
{
	return (b << 8);
}

INLINED bitboard_t	shift_down(bitboard_t b)
{
	return (b >> 8);
}

INLINED bitboard_t	shift_up_left(bitboard_t b)
{
	return ((b & ~FILE_A_BITS) << 7);
}

INLINED bitboard_t	shift_up_right(bitboard_t b)
{
	return ((b & ~FILE_H_BITS) << 9);
}

INLINED bitboard_t	shift_down_left(bitboard_t b)
{
	return ((b & ~FILE_A_BITS) >> 9);
}

INLINED bitboard_t shift_down_right(bitboard_t b)
{
	return ((b & ~FILE_H_BITS) >> 7);
}

INLINED bool		more_than_one(bitboard_t b)
{
	return (b & (b - 1));
}

INLINED bitboard_t	file_bits(file_t file)
{
	return (FILE_A_BITS << file);
}

INLINED bitboard_t	file_square_bits(square_t square)
{
	return (file_bits(file_of_square(square)));
}

INLINED bitboard_t	rank_bits(rank_t rank)
{
	return (RANK_1_BITS << (8 * rank));
}

INLINED bitboard_t	rank_square_bits(square_t square)
{
	return (rank_bits(rank_of_square(square)));
}

INLINED bitboard_t	squares_between(square_t sq1, square_t sq2)
{
	return (LineBits[sq1][sq2]
		& ((FULL_BITS << (sq1 + (sq1 < sq2)))
		^ (FULL_BITS << (sq2 + !(sq1 < sq2)))));
}

INLINED bool		aligned(square_t sq1, square_t sq2, square_t sq3)
{
	return (LineBits[sq1][sq2] & square_bit(sq3));
}

INLINED bitboard_t	bishop_move_bits(square_t square, bitboard_t occupied)
{
	const magic_t	*magic = &BishopMagics[square];

	return magic->moves[magic_index(magic, occupied)];
}

INLINED bitboard_t	rook_move_bits(square_t square, bitboard_t occupied)
{
	const magic_t	*magic = &RookMagics[square];

	return magic->moves[magic_index(magic, occupied)];
}

INLINED int			popcount(bitboard_t b)
{
#ifndef USE_POPCNT
	const bitboard_t	m1 = 0x5555555555555555ull;
	const bitboard_t	m2 = 0x3333333333333333ull;
	const bitboard_t	m4 = 0x0F0F0F0F0F0F0F0Full;
	const bitboard_t	hx = 0x0101010101010101ull;

	b -= (b >> 1) & m1;
	b = (b & m2) + ((b >> 2) & m2);
	b = (b + (b >> 4)) & m4;
	return ((b * hx) >> 56);

#elif defined(_MSC_VER) || defined (__INTEL_COMPILER)

	return (int)_mm_popcnt_u64(b);

#else

	return __builtin_popcountll(b);

#endif
}

# if defined(__GNUC__)

INLINED square_t	first_square(bitboard_t b)
{
	return (__builtin_ctzll(b));
}

INLINED square_t	last_square(bitboard_t b)
{
	return (SQ_A8 ^ __builtin_clzll(b));
}

# elif defined(_MSC_VER)

INLINED square_t	first_square(bitboard_t b)
{
	unsigned long	index;
	_BitScanForward64(&index, b);
	return ((square_t)index);
}

INLINED square_t	last_square(bitboard_t b)
{
	unsigned long	index;
	_BitScanReverse64(&index, b);
	return ((square_t)index);
}

# else
#  error "Unsupported compiler."
# endif

INLINED square_t	pop_first_square(bitboard_t *b)
{
	const square_t	square = first_square(*b);
	*b &= *b - 1;
	return (square);
}

INLINED square_t	relative_last_square(color_t c, bitboard_t b)
{
	return (c == WHITE ? last_square(b) : first_square(b));
}

#endif
