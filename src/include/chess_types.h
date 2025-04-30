/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

#include "core.h"

// Constants for the search and movegen

enum {
    MAX_PLIES = 255 - 4,
    MAX_MOVES = 256,
};

// API for the color type

typedef u8 Color;

enum {
    WHITE,
    BLACK,
    COLOR_NB
};

INLINED bool color_is_valid(Color color) {
    return color < COLOR_NB;
}

INLINED Color color_flip(Color color) {
    assert(color_is_valid(color));
    return color ^ 0b1u;
}

// API for the piece type

typedef u8 Piece;
typedef u8 Piecetype;

enum {
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

enum {
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

INLINED bool piecetype_is_valid(Piecetype piecetype) {
    return piecetype <= KING;
}

INLINED bool piece_is_valid(Piece piece) {
    return piece < PIECE_NB && piecetype_is_valid(piece & 0b111u);
}

INLINED Piece create_piece(Color color, Piecetype piecetype) {
    assert(color_is_valid(color));
    assert(piecetype_is_valid(piecetype));
    return piecetype + (color << 3);
}

INLINED Piecetype piece_type(Piece piece) {
    assert(piece_is_valid(piece));
    return piece & 0b111u;
}

INLINED Color piece_color(Piece piece) {
    assert(piece_is_valid(piece));
    return piece >> 3;
}

INLINED Piece opposite_piece(Piece piece) {
    assert(piece_is_valid(piece));
    return piece ^ 0b1000u;
}

// API for the file and rank types

typedef u8 File;
typedef u8 Rank;

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

INLINED bool file_is_valid(File file) {
    return file < FILE_NB;
}

INLINED File file_flip(File file) {
    assert(file_is_valid(file));
    return file ^ 0b111u;
}

INLINED File file_to_queenside(File file) {
    assert(file_is_valid(file));
    return u8_min(file, file_flip(file));
}

INLINED bool rank_is_valid(Rank rank) {
    return rank < RANK_NB;
}

INLINED Rank rank_flip(Rank rank) {
    assert(rank_is_valid(rank));
    return rank ^ 0b111u;
}

INLINED Rank rank_relative(Rank rank, Color color) {
    assert(rank_is_valid(rank));
    assert(color_is_valid(color));
    return rank ^ (0b111u * color);
}

// API for the square and direction types

typedef u8 Square;
typedef i8 Direction;

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

enum {
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

INLINED bool square_is_valid(Square square)
{
    return square < SQUARE_NB;
}

INLINED Square create_square(File file, Rank rank)
{
    assert(file_is_valid(file));
    assert(rank_is_valid(rank));
    return file + (rank << 3);
}

INLINED File square_file(Square square)
{
    assert(square_is_valid(square));
    return square & 0b111u;
}

INLINED Rank square_rank(Square square)
{
    assert(square_is_valid(square));
    return square >> 3;
}

INLINED u8 square_file_distance(Square square1, Square square2)
{
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    return i8_abs((i8)(square_file(square1) - square_file(square2)));
}

INLINED u8 square_rank_distance(Square square1, Square square2)
{
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    return i8_abs((i8)(square_rank(square1) - square_rank(square2)));
}

INLINED u8 square_distance(Square square1, Square square2)
{
    assert(square_is_valid(square1));
    assert(square_is_valid(square2));
    extern u8 SquareDistance[SQUARE_NB][SQUARE_NB];

    return SquareDistance[square1][square2];
}

INLINED Square square_flip(Square square)
{
    assert(square_is_valid(square));
    return square ^ 0b111000u;
}

INLINED Square square_relative(Square square, Color color)
{
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return square ^ (color * 0b111000u);
}

INLINED Rank square_rank_relative(Square square, Color color)
{
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    return rank_relative(square_rank(square), color);
}

INLINED u8 square_to_psqt_index(Square square)
{
    assert(square_is_valid(square));
    return square_rank(square) * 4 + file_to_queenside(square_file(square));
}

INLINED Direction pawn_direction(Color color)
{
    assert(color_is_valid(color));
    return color == WHITE ? NORTH : SOUTH;
}

typedef u8 CastlingRight;
typedef u8 CastlingMask;

enum
{
    WHITE_OO = 0,
    WHITE_OOO = 1,
    BLACK_OO = 2,
    BLACK_OOO = 3,
    CASTLING_NB = 4
};

enum
{
    WHITE_OO_MASK = 1,
    WHITE_OOO_MASK = 2,
    WHITE_CASTLING_MASK = 3,
    BLACK_OO_MASK = 4,
    OO_MASK = 5,
    BLACK_OOO_MASK = 8,
    OOO_MASK = 10,
    BLACK_CASTLING_MASK = 12,
    ANY_CASTLING_MASK = 15,
    CASTLING_MASK_NB = 16
};

INLINED bool clright_is_valid(CastlingRight clright) {
    return clright < CASTLING_NB;
}

INLINED CastlingRight relative_clright(Color color, bool queenside) {
    assert(color_is_valid(color));
    return (color == WHITE ? WHITE_OO : BLACK_OO) + queenside;
}

INLINED CastlingMask clright_to_clmask(CastlingRight clright) {
    assert(clright_is_valid(clright));
    return (CastlingMask)(1 << clright);
}

INLINED bool clmask_is_valid(CastlingMask clmask) {
    return clmask < CASTLING_MASK_NB;
}

INLINED CastlingMask relative_clmask(Color color) {
    assert(color_is_valid(color));
    return color == WHITE ? WHITE_CASTLING_MASK : BLACK_CASTLING_MASK;
}

INLINED CastlingRight clmask_to_clright(CastlingMask clmask) {
    assert(clmask != 0 && !(clmask & (clmask - 1)) && clmask_is_valid(clmask));
    return (CastlingRight)u64_first_one(clmask);
}

// API for the move type.

typedef u16 Move;
typedef u16 Movetype;

// clang-format off

enum
{
    NO_MOVE = 0,
    NULL_MOVE = 0b000001000001u, // b1b1

    NORMAL_MOVE   = 0b00u << 14,
    PROMOTION     = 0b01u << 14,
    EN_PASSANT    = 0b10u << 14,
    CASTLING      = 0b11u << 14,
    MOVETYPE_MASK = 0b11u << 14,

    SQUARE_MASK = 0x0FFFu,
};

// clang-format on

INLINED Square move_from(Move move) {
    return (Square)((move >> 6) & 0b111111u);
}

INLINED Square move_to(Move move) {
    return (Square)(move & 0b111111u);
}

INLINED bool move_is_valid(Move move) {
    return move_from(move) != move_to(move);
}

INLINED Move create_move(Square from, Square to) {
    assert(square_is_valid(from));
    assert(square_is_valid(to));
    return (Move)(((u16)from << 6) + to);
}

INLINED Move create_promotion_move(Square from, Square to, Piecetype piecetype) {
    assert(square_is_valid(from));
    assert(square_is_valid(to));
    assert(piecetype >= KNIGHT && piecetype <= QUEEN);
    return create_move(from, to) + PROMOTION + ((u16)(piecetype - KNIGHT) << 12);
}

INLINED Move create_en_passant_move(Square from, Square to) {
    assert(square_is_valid(from));
    assert(square_is_valid(to));
    return create_move(from, to) + EN_PASSANT;
}

INLINED Move create_castling_move(Square from, Square to) {
    assert(square_is_valid(from));
    assert(square_is_valid(to));
    return create_move(from, to) + CASTLING;
}

INLINED Move move_reverse(Move move) {
    return create_move(move_to(move), move_from(move));
}

INLINED u16 move_square_mask(Move move) {
    return move & 0x0FFFu;
}

INLINED Movetype move_type(Move move) {
    return move & MOVETYPE_MASK;
}

INLINED Piecetype move_promotion_type(Move move) {
    return (Piecetype)(((move >> 12) & 0b11u) + KNIGHT);
}

// API for the score, scorepair, phase and scalefactor types.

typedef i16 Score;
typedef i32 Scorepair;
typedef u8 Phase;
typedef i16 Scalefactor;

enum {
    DRAW = 0,
    VICTORY = 10000,
    MATE = 32000,
    MATE_FOUND = MATE - MAX_PLIES,
    INF_SCORE = 32001,
    NO_SCORE = 32002
};

enum {
    SCALE_NORMAL = 256,
    SCALE_DRAW = 0,
    SCALE_RESOLUTION = SCALE_NORMAL,
};

INLINED bool score_is_valid(Score score) {
    return i16_abs(score) <= NO_SCORE;
}

INLINED bool score_is_usable(Score score) {
    return i16_abs(score) < INF_SCORE;
}

INLINED bool score_is_mate(Score score) {
    return i16_abs(score) >= MATE_FOUND && i16_abs(score) <= MATE;
}

INLINED bool score_is_normal(Score score) {
    return i16_abs(score) < MATE_FOUND;
}

INLINED Score mate_in(u8 plies) {
    assert(plies <= MAX_PLIES);
    return MATE - (Score)plies;
}

INLINED Score mated_in(u8 plies) {
    assert(plies <= MAX_PLIES);
    return (Score)plies - MATE;
}

INLINED Score score_scaled(Score score, Scalefactor scalefactor) {
    return score * scalefactor / SCALE_NORMAL;
}

INLINED Score scorepair_midgame(Scorepair scorepair) {
    return (Score)(u16)(((u32)scorepair + 0x8000u) >> 16);
}

INLINED Score scorepair_endgame(Scorepair scorepair) {
    return (Score)(u16)(u32)scorepair;
}

INLINED bool scorepair_is_valid(Scorepair scorepair) {
    return score_is_usable(scorepair_midgame(scorepair))
        && score_is_usable(scorepair_endgame(scorepair));
}

#define SPAIR(mg, eg) (Scorepair)(((u32)(i32)mg << 16) + (u32)(i32)eg)

INLINED Scorepair create_scorepair(Score midgame, Score endgame) {
    assert(score_is_usable(midgame));
    assert(score_is_usable(endgame));
    return (Scorepair)(((u32)(i32)midgame << 16) + (u32)(i32)endgame);
}

INLINED Scorepair scorepair_multiply(Scorepair scorepair, i16 factor) {
    assert(i32_abs(scorepair_midgame(scorepair) * factor) < INF_SCORE);
    assert(i32_abs(scorepair_endgame(scorepair) * factor) < INF_SCORE);
    return scorepair * factor;
}

INLINED Scorepair scorepair_divide(Scorepair scorepair, i16 factor) {
    assert(factor != 0);
    return create_scorepair(
        scorepair_midgame(scorepair) / factor,
        scorepair_endgame(scorepair) / factor
    );
}

enum {
    MIDGAME,
    ENDGAME,
    PHASE_NB
};

// API for the score bound type.

typedef u8 Bound;

enum {
    NO_BOUND = 0,
    UPPER_BOUND,
    LOWER_BOUND,
    EXACT_BOUND
};

// Helper to compute nodes per second while avoiding division by zero issues
INLINED u64 compute_nps(u64 nodes, Duration duration) {
    u64 nodes_per_millisecond;
    u64 d;

    d = (u64)duration_max(duration, 1);
    nodes_per_millisecond = nodes / d;
    nodes -= nodes_per_millisecond * d;
    return nodes_per_millisecond * 1000 + (nodes * 1000 / d);
}

#endif
