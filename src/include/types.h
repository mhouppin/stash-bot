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

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Workaround to avoid problems with duplicated function symbols,
// when -finline is disabled or when a function inlining fails.

#define INLINED static inline

// API for basic integer operations.

INLINED int imax(int a, int b) { return a > b ? a : b; }

INLINED int imin(int a, int b) { return a < b ? a : b; }

INLINED int iclamp(int value, int lower, int upper)
{
    return value < lower ? lower : value > upper ? upper : value;
}

// API for the color type.

typedef int8_t color_t;

enum
{
    WHITE,
    BLACK,
    COLOR_NB = 2
};

INLINED color_t not_color(color_t color) { return color ^ BLACK; }

// API for the piece type.

typedef int8_t piece_t;
typedef int8_t piecetype_t;

enum
{
    NO_PIECE,
    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,
    BLACK_PAWN = 9,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,
    PIECE_NB = 16
};

enum
{
    NO_PIECETYPE,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    ALL_PIECES = 0,
    PIECETYPE_NB = 8
};

enum
{
    SCALE_NORMAL = 256,
    SCALE_DRAW = 0,
};

INLINED piecetype_t piece_type(piece_t piece) { return piece & 7; }

INLINED color_t piece_color(piece_t piece) { return piece >> 3; }

INLINED piece_t create_piece(color_t color, piecetype_t piecetype)
{
    return piecetype + (color << 3);
}

INLINED piece_t opposite_piece(piece_t piece) { return piece ^ 8; }

// API for the file, rank, square and direction types.

typedef int16_t square_t;
typedef int16_t direction_t;
typedef int8_t file_t;
typedef int8_t rank_t;

// clang-format off

enum
{
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,
    SQUARE_NB = 64
};

// clang-format on

enum
{
    NORTH = 8,
    SOUTH = -8,
    EAST = 1,
    WEST = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

// clang-format off

enum
{
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
};

enum
{
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
};

// clang-format on

extern int SquareDistance[SQUARE_NB][SQUARE_NB];

INLINED file_t sq_file(square_t square) { return square & 7; }

INLINED rank_t sq_rank(square_t square) { return square >> 3; }

INLINED square_t create_sq(file_t file, rank_t rank) { return file + (rank << 3); }

INLINED square_t opposite_sq(square_t square) { return square ^ SQ_A8; }

INLINED square_t relative_sq(square_t square, color_t color) { return square ^ (SQ_A8 * color); }

INLINED rank_t relative_rank(rank_t rank, color_t color) { return rank ^ (RANK_8 * color); }

INLINED rank_t relative_sq_rank(square_t square, color_t color)
{
    return relative_rank(sq_rank(square), color);
}

INLINED square_t to_sq32(square_t square)
{
    return sq_rank(square) * 4 + imin(sq_file(square), sq_file(square) ^ 7);
}

INLINED bool is_valid_sq(square_t square) { return square >= SQ_A1 && square <= SQ_H8; }

INLINED direction_t pawn_direction(color_t color) { return color == WHITE ? NORTH : SOUTH; }

// API for the castling rights.

enum
{
    WHITE_OO = 1,
    WHITE_OOO = 2,
    WHITE_CASTLING = 3,
    BLACK_OO = 4,
    KINGSIDE_CASTLING = 5,
    BLACK_OOO = 8,
    QUEENSIDE_CASTLING = 10,
    BLACK_CASTLING = 12,
    ANY_CASTLING = 15,
    CASTLING_NB = 16
};

INLINED int has_castling(color_t color, int castlings)
{
    return castlings & (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING);
}

// API for the move type.

typedef int32_t move_t;
typedef int32_t movetype_t;

enum
{
    NO_MOVE = 0,
    NULL_MOVE = 65,

    NORMAL_MOVE = 0,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING = 3 << 14,
    MOVETYPE_MASK = 3 << 14
};

INLINED square_t from_sq(move_t move) { return (square_t)((move >> 6) & SQ_H8); }

INLINED square_t to_sq(move_t move) { return (square_t)(move & SQ_H8); }

INLINED int square_mask(move_t move) { return move & 0xFFF; }

INLINED movetype_t move_type(move_t move) { return move & MOVETYPE_MASK; }

INLINED piecetype_t promotion_type(move_t move)
{
    return (piecetype_t)(((move >> 12) & 3) + KNIGHT);
}

INLINED move_t create_move(square_t from, square_t to) { return (move_t)((from << 6) + to); }

INLINED move_t reverse_move(move_t move) { return create_move(to_sq(move), from_sq(move)); }

INLINED move_t create_promotion(square_t from, square_t to, piecetype_t piecetype)
{
    return (move_t)(PROMOTION + ((piecetype - KNIGHT) << 12) + (from << 6) + to);
}

INLINED move_t create_en_passant(square_t from, square_t to)
{
    return (move_t)(EN_PASSANT + (from << 6) + to);
}

INLINED move_t create_castling(square_t from, square_t to)
{
    return (move_t)(CASTLING + (from << 6) + to);
}

INLINED bool is_valid_move(move_t move) { return from_sq(move) != to_sq(move); }

// API for the score, scorepair and phase types.

typedef int16_t score_t;
typedef int32_t scorepair_t;
typedef int16_t phase_t;

enum
{
    MAX_PLIES = 238
};

enum
{
    DRAW = 0,
    VICTORY = 10000,
    MATE = 32000,
    MATE_FOUND = MATE - MAX_PLIES,
    INF_SCORE = 32001,
    NO_SCORE = 32002
};

enum
{
    NO_BOUND = 0,
    UPPER_BOUND,
    LOWER_BOUND,
    EXACT_BOUND
};

enum
{
    MIDGAME,
    ENDGAME,
    PHASE_NB
};

INLINED score_t midgame_score(scorepair_t pair)
{
    return (score_t)(uint16_t)(((uint32_t)pair + 32768) >> 16);
}

INLINED score_t endgame_score(scorepair_t pair) { return (score_t)(uint16_t)(uint32_t)pair; }

#define SPAIR(mg, eg) ((scorepair_t)((uint32_t)(mg) << 16) + (eg))

INLINED scorepair_t create_scorepair(score_t midgame, score_t endgame)
{
    return (scorepair_t)((uint32_t)midgame << 16) + endgame;
}

INLINED scorepair_t scorepair_multiply(scorepair_t s, int i) { return s * i; }

INLINED scorepair_t scorepair_divide(scorepair_t s, int i)
{
    return create_scorepair(midgame_score(s) / i, endgame_score(s) / i);
}

INLINED score_t mate_in(int ply) { return MATE - ply; }

INLINED score_t mated_in(int ply) { return ply - MATE; }

#endif // TYPES_H
