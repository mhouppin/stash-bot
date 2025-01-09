/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include "chess_types.h"

typedef u64 Bitboard;

// Constants for file bitboard masks
static const Bitboard FILE_A_BB = U64(0x0101010101010101);
static const Bitboard FILE_B_BB = U64(0x0202020202020202);
static const Bitboard FILE_C_BB = U64(0x0404040404040404);
static const Bitboard FILE_D_BB = U64(0x0808080808080808);
static const Bitboard FILE_E_BB = U64(0x1010101010101010);
static const Bitboard FILE_F_BB = U64(0x2020202020202020);
static const Bitboard FILE_G_BB = U64(0x4040404040404040);
static const Bitboard FILE_H_BB = U64(0x8080808080808080);

// Constants for rank bitboard masks
static const Bitboard RANK_1_BB = U64(0x00000000000000FF);
static const Bitboard RANK_2_BB = U64(0x000000000000FF00);
static const Bitboard RANK_3_BB = U64(0x0000000000FF0000);
static const Bitboard RANK_4_BB = U64(0x00000000FF000000);
static const Bitboard RANK_5_BB = U64(0x000000FF00000000);
static const Bitboard RANK_6_BB = U64(0x0000FF0000000000);
static const Bitboard RANK_7_BB = U64(0x00FF000000000000);
static const Bitboard RANK_8_BB = U64(0xFF00000000000000);

// Constants for miscellaneous bitboard masks
static const Bitboard ALL_BB = U64(0xFFFFFFFFFFFFFFFF);
static const Bitboard DSQ_BB = U64(0xAA55AA55AA55AA55);
static const Bitboard LSQ_BB = ~DSQ_BB;
static const Bitboard KINGSIDE_BB = U64(0xF0F0F0F0F0F0F0F0);
static const Bitboard QUEENSIDE_BB = U64(0x0F0F0F0F0F0F0F0F);
static const Bitboard CENTER_FILES_BB = U64(0x3C3C3C3C3C3C3C3C);
static const Bitboard CENTER_BB = U64(0x0000001818000000);

// The structure used for magic bitboards
typedef struct _Magic {
    Bitboard mask;
    Bitboard magic;
    Bitboard *moves;
    u32 shift;
} Magic;

// This function returns the index of the attack bitboard for a given magic and occupancy bitboard.
// Avoid using this function directly, use the helpers for bishop/rook/queen attacks instead.
u32 magic_index(const Magic *magic, Bitboard occupancy);

// Initializes all bitboard tables and magic bitboards
void bitboard_init(void);

// Returns the bitboard representing the given square
INLINED Bitboard square_bb(Square square) {
    assert(square_is_valid(square));
    return (Bitboard)1 << square;
}

// Returns the bitboard representing the given file
INLINED Bitboard file_bb(File file) {
    assert(file_is_valid(file));
    return FILE_A_BB << file;
}

// Returns the bitboard representing the given square file
INLINED Bitboard square_file_bb(Square square) {
    assert(square_is_valid(square));
    return file_bb(square_file(square));
}

// Returns the bitboard representing the given rank
INLINED Bitboard rank_bb(File rank) {
    assert(rank_is_valid(rank));
    return RANK_1_BB << (8 * rank);
}

// Returns the bitboard representing the given square rank
INLINED Bitboard square_rank_bb(Square square) {
    assert(square_is_valid(square));
    return rank_bb(square_rank(square));
}

// Sets the given square in the bitboard
INLINED void bb_set_square(Bitboard *bb, Square square) {
    assert(square_is_valid(square));
    *bb |= square_bb(square);
}

// Clears the given square in the bitboard
INLINED void bb_reset_square(Bitboard *bb, Square square) {
    assert(square_is_valid(square));
    *bb &= ~square_bb(square);
}

// Flips the given square's state in the bitboard
INLINED void bb_flip_square(Bitboard *bb, Square square) {
    assert(square_is_valid(square));
    *bb ^= square_bb(square);
}

// Checks if the given square is set in the bitboard
INLINED bool bb_square_is_set(Bitboard bb, Square square) {
    return bb & square_bb(square);
}

// Flips the bitboard on the horizontal axis
INLINED Bitboard bb_flip(Bitboard bb) {
    return u64_flip_bytes(bb);
}

// Returns the bitboard relative to the given's color POV
INLINED Bitboard bb_relative(Bitboard bb, Color color) {
    assert(color_is_valid(color));
    return (color == WHITE) ? bb : bb_flip(bb);
}

// Shifts the bitboard up by one rank
INLINED Bitboard bb_shift_up(Bitboard bb) {
    return bb << 8;
}

// Shifts the bitboard down by one rank
INLINED Bitboard bb_shift_down(Bitboard bb) {
    return bb >> 8;
}

// Shifts the bitboard left by one file
INLINED Bitboard bb_shift_left(Bitboard bb) {
    return (bb & ~FILE_A_BB) >> 1;
}

// Shifts the bitboard right by one file
INLINED Bitboard bb_shift_right(Bitboard bb) {
    return (bb & ~FILE_H_BB) << 1;
}

// Shifts the bitboard up and left
INLINED Bitboard bb_shift_up_left(Bitboard bb) {
    return (bb & ~FILE_A_BB) << 7;
}

// Shifts the bitboard up and right
INLINED Bitboard bb_shift_up_right(Bitboard bb) {
    return (bb & ~FILE_H_BB) << 9;
}

// Shifts the bitboard down and left
INLINED Bitboard bb_shift_down_left(Bitboard bb) {
    return (bb & ~FILE_A_BB) >> 9;
}

// Shifts the bitboard down and right
INLINED Bitboard bb_shift_down_right(Bitboard bb) {
    return (bb & ~FILE_H_BB) >> 7;
}

// Shifts the bitboard up relative to the color's POV
INLINED Bitboard bb_shift_up_relative(Bitboard bb, Color color) {
    assert(color_is_valid(color));
    return (color == WHITE) ? bb_shift_up(bb) : bb_shift_down(bb);
}

// Shifts the bitboard down relative to the color's POV
INLINED Bitboard bb_shift_down_relative(Bitboard bb, Color color) {
    assert(color_is_valid(color));
    return (color == WHITE) ? bb_shift_down(bb) : bb_shift_up(bb);
}

// Checks if more than one bit/square is set in the bitboard
INLINED bool bb_more_than_one(Bitboard bb) {
    return bb & (bb - 1);
}

// Returns the horizontal/vertical/diagonal line bitboard passing through both squares
INLINED Bitboard line_bb(Square square1, Square square2) {
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];

    return LineBB[square1][square2];
}

// Returns the bitboard of all squares between two squares (both squares excluded)
INLINED Bitboard between_squares_bb(Square square1, Square square2) {
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    return line_bb(square1, square2)
        & ((ALL_BB << (square1 + (square1 < square2)))
           ^ (ALL_BB << (square2 + !(square1 < square2))));
}

// Checks if all three squares share the same file, rank or diagonal
INLINED bool squares_are_aligned(Square square1, Square square2, Square square3) {
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    assert(square_is_valid(square3));
    return line_bb(square1, square2) & square_bb(square3);
}

// Returns the bitboard of the files adjacent to the given square
INLINED Bitboard adjacent_files_bb(Square square) {
    assert(square_is_valid(square));
    Bitboard bb = square_file_bb(square);
    return bb_shift_left(bb) | bb_shift_right(bb);
}

// Returns the bitboard of all squares above the given square from the color's POV
INLINED Bitboard forward_ranks_bb(Square square, Color color) {
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return (color == WHITE) ? ~RANK_1_BB << 8 * square_rank(square)
                            : ~RANK_8_BB >> 8 * (RANK_8 - square_rank(square));
}

// Returns the bitboard of all squares in front of the given square from the color's POV
INLINED Bitboard forward_file_bb(Square square, Color color) {
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return forward_ranks_bb(square, color) & square_file_bb(square);
}

// Returns the bitboard of all squares still attackable by the pawn from the given square and color
INLINED Bitboard pawn_attack_span_bb(Square square, Color color) {
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return forward_ranks_bb(square, color) & adjacent_files_bb(square);
}

// Returns the bitboard of all squares where an opponent pawn would prevent a pawn from being
// considered "passed"
INLINED Bitboard passed_pawn_span_bb(Square square, Color color) {
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return forward_ranks_bb(square, color) & (adjacent_files_bb(square) | square_file_bb(square));
}

// Returns the number of bits/squares set in the bitboard
INLINED u32 bb_popcount(Bitboard bb) {
    return u64_count_ones(bb);
}

// Returns the first square set in the bitboard
INLINED Square bb_first_square(Bitboard bb) {
    assert(bb != 0);
    return u64_first_one(bb);
}

// Returns the last square set in the bitboard
INLINED Square bb_last_square(Bitboard bb) {
    assert(bb != 0);
    return u64_last_one(bb);
}

// Returns the first square set in the bitboard relative to the color's POV
INLINED Square bb_first_square_relative(Bitboard bb, Color us) {
    assert(bb != 0);
    return us == WHITE ? bb_first_square(bb) : bb_last_square(bb);
}

// Returns the last square set in the bitboard relative to the color's POV
INLINED Square bb_last_square_relative(Bitboard bb, Color us) {
    assert(bb != 0);
    return us == WHITE ? bb_last_square(bb) : bb_first_square(bb);
}

// Removes and returns the first square set in the bitboard
INLINED Square bb_pop_first_square(Bitboard *bb) {
    Square square = bb_first_square(*bb);
    *bb &= *bb - 1;
    return square;
}

#endif
