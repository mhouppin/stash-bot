/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#include "psq_score.h"
#include "types.h"

// clang-format off

scorepair_t PsqScore[PIECE_NB][SQUARE_NB];
const score_t PieceScores[PHASE_NB][PIECE_NB] = {
    {
        0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE, ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0,
        0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE, ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0
    },
    {
        0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE, ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0,
        0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE, ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0
    }
};

#define S SPAIR

const scorepair_t PawnBonus[RANK_NB][FILE_NB] = {
    { },
    { S(-10, 25), S( -5, 23), S(-13, 17), S(  0,  5), S(  1, 18), S( 29, 17), S( 34,  6), S( 10,-16) },
    { S( -9, 18), S(-17, 24), S( -1,  9), S(  2,  3), S(  8, 10), S(  4, 14), S( 20, -9), S( 12,-14) },
    { S(-10, 25), S( -8, 17), S(  6,-10), S( 10,-21), S( 15,-18), S( 16, -7), S( 15, -9), S(  4, -8) },
    { S( -4, 46), S( -3, 28), S(  2,  7), S( 23,-25), S( 35,-17), S( 43,-17), S( 23,  2), S( 10, 11) },
    { S(  3, 61), S(  8, 46), S( 19, 17), S( 28,-20), S( 47,-28), S(110, -9), S( 70, 14), S( 29, 28) },
    { S( 92, 26), S( 81, 25), S( 90,  1), S(103,-50), S(124,-54), S( 73,-18), S(-73, 45), S(-45, 53) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -67, -46), S( -17, -31), S( -28, -12), S( -19,  13) },
        { S( -20, -10), S( -28,  14), S( -13,  -7), S(  -7,   5) },
        { S( -20, -25), S(  -7,   1), S(  -5,   8), S(   4,  31) },
        { S(  -8,  22), S(  13,  31), S(   5,  45), S(   7,  51) },
        { S(  10,  33), S(   2,  34), S(  20,  45), S(   7,  63) },
        { S( -21,  17), S(   8,  30), S(  30,  42), S(  26,  50) },
        { S( -10,   2), S( -19,  20), S(  32,   9), S(  42,  26) },
        { S(-176, -44), S(-122,  35), S(-115,  62), S(   9,  22) }
    },

    // Bishop
    {
        { S(   4,  -2), S(  -3,   6), S( -17,   6), S( -24,  22) },
        { S(   2,  -1), S(   3,   3), S(   0,   9), S( -13,  25) },
        { S(  -4,  10), S(   4,  27), S(  -2,  36), S(  -5,  45) },
        { S(  -6,   7), S(  -1,  28), S(  -7,  51), S(  15,  55) },
        { S( -14,  21), S(  -5,  48), S(  13,  40), S(  16,  64) },
        { S(  -2,  23), S(   7,  49), S(  22,  41), S(  16,  37) },
        { S( -42,  41), S( -37,  41), S( -21,  40), S( -16,  43) },
        { S( -64,  58), S( -58,  48), S(-151,  68), S(-118,  63) }
    },

    // Rook
    {
        { S( -31,   5), S( -26,   9), S( -21,  13), S( -19,  10) },
        { S( -56,   8), S( -41,   8), S( -30,   8), S( -31,  15) },
        { S( -45,  20), S( -32,  24), S( -38,  28), S( -41,  31) },
        { S( -53,  49), S( -39,  53), S( -36,  48), S( -24,  39) },
        { S( -29,  62), S( -11,  70), S(  -2,  61), S(  15,  54) },
        { S( -22,  69), S(  17,  57), S(  21,  64), S(  43,  48) },
        { S(  -2,  74), S(  -9,  76), S(  26,  67), S(  35,  66) },
        { S(  15,  63), S(  19,  65), S(   7,  69), S(  13,  62) }
    },

    // Queen
    {
        { S(  13, -53), S(  11, -69), S(  14, -81), S(  19, -49) },
        { S(  13, -49), S(  21, -56), S(  21, -44), S(  20, -36) },
        { S(  13, -26), S(  14,  -1), S(   8,  25), S(   4,  22) },
        { S(   3,  29), S(   9,  44), S(  -1,  60), S(   1,  73) },
        { S(  11,  36), S(  -3,  83), S(   1,  95), S(  -6, 110) },
        { S(   7,  41), S(  19,  65), S(   1, 116), S(  -4, 122) },
        { S(  -9,  59), S( -49, 103), S(   1, 113), S( -35, 150) },
        { S( -11,  73), S( -26,  93), S( -16, 113), S( -26, 113) }
    },

    // King
    {
        { S( 276,  -1), S( 291,  45), S( 222,  72), S( 171,  92) },
        { S( 284,  47), S( 273,  80), S( 245,  99), S( 220, 108) },
        { S( 194,  82), S( 255,  90), S( 236, 112), S( 225, 124) },
        { S( 156,  93), S( 245, 111), S( 238, 128), S( 221, 142) },
        { S( 196, 114), S( 277, 146), S( 260, 156), S( 228, 165) },
        { S( 231, 142), S( 320, 176), S( 298, 179), S( 284, 166) },
        { S( 217, 100), S( 276, 181), S( 304, 166), S( 295, 153) },
        { S( 279,-154), S( 358,  48), S( 331,  77), S( 272, 102) }
    }
};

#undef S

// clang-format on

void psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t pieceValue =
            create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

        for (square_t square = SQ_A1; square <= SQ_H8; ++square)
        {
            scorepair_t psqEntry;

            if (piece == WHITE_PAWN)
                psqEntry = pieceValue + PawnBonus[sq_rank(square)][sq_file(square)];

            else
            {
                file_t queensideFile = min(sq_file(square), sq_file(square) ^ 7);

                psqEntry = pieceValue + PieceBonus[piece][sq_rank(square)][queensideFile];
            }

            PsqScore[piece][square] = psqEntry;
            PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
        }
    }
}
