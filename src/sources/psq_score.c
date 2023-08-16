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

// Square-based Pawn scoring for evaluation
const scorepair_t PawnBonus[RANK_NB][FILE_NB] = {
    { },
    { S(-35, 18), S(-30, 16), S(-41,  7), S(-23, -4), S(-24,  6), S( 13, 14), S( 15,  7), S(-19,-21) },
    { S(-32, 10), S(-44, 13), S(-22,  0), S(-20, -3), S(-15,  5), S(-19,  7), S( -4,-11), S(-18,-19) },
    { S(-33, 15), S(-33,  9), S(-15,-16), S( -9,-28), S( -1,-21), S( -4,-13), S( -9,-13), S(-27,-17) },
    { S(-23, 36), S(-27, 19), S(-17, -4), S(  4,-29), S( 19,-20), S( 26,-19), S( -2, -1), S(-11,  3) },
    { S(-15, 54), S(-14, 41), S(  8,  7), S( 26,-29), S( 32,-25), S(101, -2), S( 45, 20), S( 11, 23) },
    { S( 82, 21), S( 64, 22), S( 65,  0), S( 86,-40), S( 98,-39), S( 44,-19), S(-79, 23), S(-66, 25) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -51, -46), S(  -2, -57), S( -17, -27), S(  -3,  -9) },
        { S(  -9, -27), S( -11, -10), S(   4, -27), S(   8, -11) },
        { S(  -3, -37), S(   8, -16), S(  14, -10), S(  25,  15) },
        { S(   8,   6), S(  31,  15), S(  24,  26), S(  28,  34) },
        { S(  19,  22), S(  19,  21), S(  45,  25), S(  24,  43) },
        { S( -25,  12), S(  18,  19), S(  39,  27), S(  47,  28) },
        { S(   6, -11), S( -14,   6), S(  54,   0), S(  54,  15) },
        { S(-162, -58), S(-109,   7), S(-102,  25), S(  29,   8) }
    },

    // Bishop
    {
        { S(  12, -51), S(  29, -27), S(  -2, -16), S(  -4, -14) },
        { S(  18, -33), S(  19, -37), S(  22, -13), S(   2,  -1) },
        { S(  15, -17), S(  24,  -4), S(  11, -13), S(  13,  23) },
        { S(  17, -30), S(  20,   7), S(  14,  28), S(  33,  40) },
        { S(   2,  -7), S(  18,  22), S(  36,  25), S(  40,  45) },
        { S(  23,  -4), S(  25,  24), S(  38,   1), S(  41,  18) },
        { S( -54,  -1), S( -50,  -8), S(  -8,  17), S(  -5,  10) },
        { S( -58, -15), S( -43,  10), S(-137,  17), S(-100,  15) }
    },

    // Rook
    {
        { S( -23, -42), S( -21, -36), S( -16, -32), S( -13, -35) },
        { S( -51, -40), S( -29, -39), S( -16, -34), S( -16, -34) },
        { S( -38, -25), S( -17, -22), S( -29, -16), S( -24, -18) },
        { S( -34,  -3), S( -21,   4), S( -23,   4), S( -11,  -8) },
        { S( -13,  18), S(  -1,  26), S(  21,  16), S(  31,  16) },
        { S(  -2,  28), S(  30,  23), S(  38,  26), S(  60,  15) },
        { S(  14,  32), S(   7,  33), S(  48,  31), S(  60,  31) },
        { S(  25,  26), S(  34,  29), S(  17,  27), S(  25,  23) }
    },

    // Queen
    {
        { S(   3, -84), S(  -9, -94), S(  -2,-109), S(   7, -87) },
        { S(   4, -77), S(  13, -80), S(  17, -76), S(  12, -52) },
        { S(  10, -49), S(  14, -32), S(   4,  -8), S(  -2,  -7) },
        { S(   4,  -6), S(  19,  -2), S(  -1,  25), S(  -5,  43) },
        { S(  19,  -2), S(   3,  44), S(   9,  43), S(  -0,  63) },
        { S(  11,   8), S(  10,  36), S(   4,  73), S(   5,  70) },
        { S(  -9,  19), S( -44,  50), S(   0,  67), S( -16,  92) },
        { S( -12,  30), S( -24,  42), S( -17,  61), S( -19,  67) }
    },

    // King
    {
        { S(  31,-104), S(  54, -56), S( -31, -41), S( -94, -27) },
        { S(  34, -53), S(  19, -20), S(  -9,  -7), S( -33,  -3) },
        { S( -64, -39), S(   2, -17), S( -19,   4), S( -21,  14) },
        { S(-113, -33), S( -23,   4), S( -21,  20), S( -30,  30) },
        { S( -67,  -2), S(  18,  43), S(   7,  50), S( -19,  52) },
        { S( -25,  29), S(  58,  78), S(  43,  81), S(  32,  63) },
        { S( -38,  -1), S(  17,  75), S(  43,  68), S(  38,  56) },
        { S(  26,-243), S( 105, -27), S(  77,   1), S(  17,  16) }
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

            // Locate the square entry based on the piece type and square.
            if (piece == WHITE_PAWN)
                psqEntry = pieceValue + PawnBonus[sq_rank(square)][sq_file(square)];

            else
            {
                // Map squares for pieces on the queenside.
                file_t queensideFile = imin(sq_file(square), sq_file(square) ^ 7);

                psqEntry = pieceValue + PieceBonus[piece][sq_rank(square)][queensideFile];
            }

            // Assign the score twice, once for White and once for Black (with
            // the square mirrored horizontally).
            PsqScore[piece][square] = psqEntry;
            PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
        }
    }
}
