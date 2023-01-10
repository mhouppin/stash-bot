/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BITBOARD_H
#define BITBOARD_H

#if (defined(USE_PREFETCH) || defined(USE_POPCNT) || defined(USE_PEXT))
// Do not include the header if unspecified, because some compilers might
// not have it.
#include <immintrin.h>
#endif

#include "types.h"

typedef uint64_t bitboard_t;

// Defines for file bitboard masks.
#define FILE_A_BB 0x0101010101010101ull
#define FILE_B_BB 0x0202020202020202ull
#define FILE_C_BB 0x0404040404040404ull
#define FILE_D_BB 0x0808080808080808ull
#define FILE_E_BB 0x1010101010101010ull
#define FILE_F_BB 0x2020202020202020ull
#define FILE_G_BB 0x4040404040404040ull
#define FILE_H_BB 0x8080808080808080ull

// Defines for rank bitboard masks.
#define RANK_1_BB 0x00000000000000FFull
#define RANK_2_BB 0x000000000000FF00ull
#define RANK_3_BB 0x0000000000FF0000ull
#define RANK_4_BB 0x00000000FF000000ull
#define RANK_5_BB 0x000000FF00000000ull
#define RANK_6_BB 0x0000FF0000000000ull
#define RANK_7_BB 0x00FF000000000000ull
#define RANK_8_BB 0xFF00000000000000ull

// Defines for miscellaneous bitboard masks.
#define FULL_BB 0xFFFFFFFFFFFFFFFFull
#define DARK_SQUARES 0xAA55AA55AA55AA55ull
#define KINGSIDE_BB 0xF0F0F0F0F0F0F0F0ull
#define QUEENSIDE_BB 0x0F0F0F0F0F0F0F0Full
#define CENTER_FILES_BB 0x3C3C3C3C3C3C3C3Cull
#define CENTER_BB 0x0000001818000000ull

// Bitboards of all squares on the line between two squares.
extern bitboard_t LineBB[SQUARE_NB][SQUARE_NB];
// Bitboards of pseudo-legal moves for a given square and piece type (not checking occupancy).
extern bitboard_t PseudoMoves[PIECETYPE_NB][SQUARE_NB];
// Bitboards of pawn capture moves.
extern bitboard_t PawnMoves[COLOR_NB][SQUARE_NB];

// The structure for magic bitboards.
typedef struct _Magic
{
    bitboard_t mask;
    bitboard_t magic;
    bitboard_t *moves;
    unsigned int shift;
} Magic;

// Returns the index of the attack bitboard for a given magic and occupancy bitboard.
INLINED unsigned int magic_index(const Magic *magic, bitboard_t occupied)
{
#ifdef USE_PEXT
    return (_pext_u64(occupied, magic->mask));
#else
    return ((unsigned int)(((occupied & magic->mask) * magic->magic) >> magic->shift));
#endif
}

// Globals for Rook and Bishop magic bitboards.
extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];

// Initializes all global bitboard tables and magic bitboards.
void bitboard_init(void);

// Returns the bitboard representing the given square.
INLINED bitboard_t square_bb(square_t square) { return ((bitboard_t)1 << square); }

// Shifts the bitboard up by one file.
INLINED bitboard_t shift_up(bitboard_t b) { return (b << 8); }

// Shifts the bitboard down by one file.
INLINED bitboard_t shift_down(bitboard_t b) { return (b >> 8); }

// Shifts the bitboard left by one rank.
INLINED bitboard_t shift_left(bitboard_t b) { return ((b & ~FILE_A_BB) >> 1); }

// Shifts the bitboard right by one rank.
INLINED bitboard_t shift_right(bitboard_t b) { return ((b & ~FILE_H_BB) << 1); }

// Shifts the bitboard up and left by one square.
INLINED bitboard_t shift_up_left(bitboard_t b) { return ((b & ~FILE_A_BB) << 7); }

// Shifts the bitboard up and right by one square.
INLINED bitboard_t shift_up_right(bitboard_t b) { return ((b & ~FILE_H_BB) << 9); }

// Shifts the bitboard down and left by one square.
INLINED bitboard_t shift_down_left(bitboard_t b) { return ((b & ~FILE_A_BB) >> 9); }

// Shifts the bitboard down and right by one square.
INLINED bitboard_t shift_down_right(bitboard_t b) { return ((b & ~FILE_H_BB) >> 7); }

// Shifts the bitboard up by one file from the given color.
INLINED bitboard_t relative_shift_up(bitboard_t b, color_t c)
{
    return ((c == WHITE) ? shift_up(b) : shift_down(b));
}

// Shifts the bitboard down by one file from the given color.
INLINED bitboard_t relative_shift_down(bitboard_t b, color_t c)
{
    return ((c == WHITE) ? shift_down(b) : shift_up(b));
}

// Checks if more than one bit is set in the bitboard.
INLINED bool more_than_one(bitboard_t b) { return (b & (b - 1)); }

// Returns the bitboard representing the given file.
INLINED bitboard_t file_bb(file_t file) { return (FILE_A_BB << file); }

// Returns the bitboard representing the given square file.
INLINED bitboard_t sq_file_bb(square_t square) { return (file_bb(sq_file(square))); }

// Returns the bitboard representing the given rank.
INLINED bitboard_t rank_bb(rank_t rank) { return (RANK_1_BB << (8 * rank)); }

// Returns the bitboard representing the given square rank.
INLINED bitboard_t sq_rank_bb(square_t square) { return (rank_bb(sq_rank(square))); }

// Returns the bitboard of all squares between two squares (both squares excluded).
INLINED bitboard_t between_bb(square_t sq1, square_t sq2)
{
    return (
        LineBB[sq1][sq2] & ((FULL_BB << (sq1 + (sq1 < sq2))) ^ (FULL_BB << (sq2 + !(sq1 < sq2)))));
}

// Checks if all three squares share the same file, rank or diagonal.
INLINED bool sq_aligned(square_t sq1, square_t sq2, square_t sq3)
{
    return (LineBB[sq1][sq2] & square_bb(sq3));
}

// Returns the bitboard of all bishop reachable squares from a given square and occupancy bitboard.
INLINED bitboard_t bishop_moves_bb(square_t square, bitboard_t occupied)
{
    const Magic *magic = &BishopMagics[square];

    return (magic->moves[magic_index(magic, occupied)]);
}

// Returns the bitboard of all rook reachable squares from a given square and occupancy bitboard.
INLINED bitboard_t rook_moves_bb(square_t square, bitboard_t occupied)
{
    const Magic *magic = &RookMagics[square];

    return (magic->moves[magic_index(magic, occupied)]);
}

// Returns the bitboard of all squares attacked by white pawns from the given bitboard.
INLINED bitboard_t wpawns_attacks_bb(bitboard_t b)
{
    return (shift_up_left(b) | shift_up_right(b));
}

// Returns the bitboard of all squares attacked by black pawns from the given bitboard.
INLINED bitboard_t bpawns_attacks_bb(bitboard_t b)
{
    return (shift_down_left(b) | shift_down_right(b));
}

// Returns the bitboard of all squares attacked twice by white pawns from the given bitboard.
INLINED bitboard_t wpawns_2attacks_bb(bitboard_t b)
{
    return (shift_up_left(b) & shift_up_right(b));
}

// Returns the bitboard of all squares attacked twice by black pawns from the given bitboard.
INLINED bitboard_t bpawns_2attacks_bb(bitboard_t b)
{
    return (shift_down_left(b) & shift_down_right(b));
}

// Returns the bitboard of the files adjacent to the given square.
INLINED bitboard_t adjacent_files_bb(square_t s)
{
    bitboard_t fileBB = sq_file_bb(s);
    return (shift_left(fileBB) | shift_right(fileBB));
}

// Returns the bitboard of all squares "higher" than the given square from the given color.
INLINED bitboard_t forward_ranks_bb(color_t c, square_t s)
{
    if (c == WHITE)
        return (~RANK_1_BB << 8 * sq_rank(s));
    else
        return (~RANK_8_BB >> 8 * (RANK_8 - sq_rank(s)));
}

// Returns the bitboard of all squares "in front" of the given square from the given color.
INLINED bitboard_t forward_file_bb(color_t c, square_t s)
{
    return (forward_ranks_bb(c, s) & sq_file_bb(s));
}

// Returns the bitboard of all squares still attackable by the pawn from the given square and color.
INLINED bitboard_t pawn_attack_span_bb(color_t c, square_t s)
{
    return (forward_ranks_bb(c, s) & adjacent_files_bb(s));
}

// Returns the bitboard of all squares where an opponent pawn would stop a given pawn from promoting
// (by attacking it or by being in front of it).
INLINED bitboard_t passed_pawn_span_bb(color_t c, square_t s)
{
    return (forward_ranks_bb(c, s) & (adjacent_files_bb(s) | sq_file_bb(s)));
}

// Returns the number of bits set in the bitboard.
INLINED int popcount(bitboard_t b)
{
// Fall back to "software" popcount for old machines.
#ifndef USE_POPCNT
    const bitboard_t m1 = 0x5555555555555555ull;
    const bitboard_t m2 = 0x3333333333333333ull;
    const bitboard_t m4 = 0x0F0F0F0F0F0F0F0Full;
    const bitboard_t hx = 0x0101010101010101ull;

    b -= (b >> 1) & m1;
    b = (b & m2) + ((b >> 2) & m2);
    b = (b + (b >> 4)) & m4;
    return ((b * hx) >> 56);

// Use the Intel intrinsic for MSVC and ICC.
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

    return ((int)_mm_popcnt_u64(b));

// Assume GCC or Clang (or more generally a compiler supporting GNU extensions).
#else

    return (__builtin_popcountll(b));

#endif
}

// Use GNU extensions' builtins if possible.
#if defined(__GNUC__)

// Returns the index of the first bit set in the bitboard.
INLINED square_t bb_first_sq(bitboard_t b) { return (__builtin_ctzll(b)); }

// Returns the index of the last bit set in the bitboard.
INLINED square_t bb_last_sq(bitboard_t b) { return (SQ_H8 ^ __builtin_clzll(b)); }

// Use the Windows intrinsics for MSVC.
#elif defined(_MSC_VER)

// Returns the index of the first bit set in the bitboard.
INLINED square_t bb_first_sq(bitboard_t b)
{
    unsigned long index;
    _BitScanForward64(&index, b);
    return ((square_t)index);
}

// Returns the index of the last bit set in the bitboard.
INLINED square_t bb_last_sq(bitboard_t b)
{
    unsigned long index;
    _BitScanReverse64(&index, b);
    return ((square_t)index);
}

#else
#error "Unsupported compiler."
#endif

// Pops the first bit set from the bitboard and returns its index.
INLINED square_t bb_pop_first_sq(bitboard_t *b)
{
    const square_t square = bb_first_sq(*b);
    *b &= *b - 1;
    return (square);
}

// Returns the index of the last bit set in the bitboard from the given color.
INLINED square_t bb_relative_last_sq(color_t c, bitboard_t b)
{
    return (c == WHITE ? bb_last_sq(b) : bb_first_sq(b));
}

// Prefetches the given address.
INLINED void prefetch(void *ptr __attribute__((unused)))
{
#ifdef USE_PREFETCH
    _mm_prefetch(ptr, _MM_HINT_T0);
#elif defined(__GNUC__)
    __builtin_prefetch(ptr);
#endif
}

#endif // BITBOARD_H
