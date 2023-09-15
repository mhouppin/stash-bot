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
    { S(-33, 17), S(-30, 16), S(-41,  7), S(-24, -2), S(-25,  7), S( 13, 15), S( 16,  7), S(-17,-23) },
    { S(-31,  9), S(-44, 13), S(-23,  1), S(-20, -2), S(-16,  6), S(-19,  7), S( -3,-11), S(-16,-20) },
    { S(-33, 15), S(-34,  9), S(-16,-16), S(-10,-28), S( -2,-21), S( -3,-13), S( -8,-13), S(-26,-17) },
    { S(-23, 36), S(-27, 19), S(-18, -4), S(  4,-30), S( 19,-20), S( 26,-20), S( -2, -1), S(-11,  2) },
    { S(-15, 54), S(-15, 41), S(  7,  7), S( 25,-29), S( 31,-25), S(101, -2), S( 46, 21), S( 11, 23) },
    { S( 82, 22), S( 64, 23), S( 65,  1), S( 87,-39), S( 97,-39), S( 44,-18), S(-78, 24), S(-66, 26) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -51, -45), S(  -1, -59), S( -17, -27), S(  -5,  -8) },
        { S(  -8, -27), S( -11, -10), S(   5, -26), S(   9, -10) },
        { S(  -2, -37), S(   8, -16), S(  15, -10), S(  25,  15) },
        { S(   9,   7), S(  32,  15), S(  24,  26), S(  28,  35) },
        { S(  21,  22), S(  20,  21), S(  45,  26), S(  25,  43) },
        { S( -25,  13), S(  19,  19), S(  40,  28), S(  48,  29) },
        { S(   6, -11), S( -14,   7), S(  55,   1), S(  54,  16) },
        { S(-163, -58), S(-109,   6), S(-102,  25), S(  29,   8) }
    },

    // Bishop
    {
        { S(  14, -51), S(  28, -27), S(  -0, -19), S(  -6, -12) },
        { S(  18, -34), S(  21, -35), S(  24, -12), S(   5,  -1) },
        { S(  15, -17), S(  24,  -3), S(  14, -10), S(  16,  24) },
        { S(  16, -30), S(  21,   8), S(  14,  30), S(  36,  41) },
        { S(   1,  -7), S(  18,  23), S(  37,  26), S(  41,  46) },
        { S(  21,  -4), S(  26,  25), S(  39,   3), S(  40,  19) },
        { S( -57,  -0), S( -49,  -6), S(  -9,  18), S(  -6,  11) },
        { S( -59, -16), S( -43,  10), S(-137,  18), S(-100,  16) }
    },

    // Rook
    {
        { S( -21, -45), S( -17, -36), S(  -9, -31), S(  -4, -35) },
        { S( -52, -43), S( -29, -40), S( -17, -35), S( -17, -35) },
        { S( -41, -27), S( -19, -22), S( -31, -16), S( -26, -19) },
        { S( -38,  -4), S( -24,   4), S( -26,   4), S( -14,  -8) },
        { S( -17,  17), S(  -3,  24), S(  18,  15), S(  29,  15) },
        { S(  -4,  26), S(  28,  21), S(  37,  25), S(  58,  14) },
        { S(  12,  31), S(   6,  33), S(  49,  31), S(  60,  32) },
        { S(  24,  25), S(  35,  29), S(  18,  27), S(  26,  23) }
    },

    // Queen
    {
        { S(   8, -82), S(   4, -92), S(  10,-108), S(  15, -74) },
        { S(   5, -77), S(  14, -80), S(  19, -74), S(  15, -56) },
        { S(   9, -48), S(  13, -34), S(   7,  -9), S(  -2,  -8) },
        { S(   3,  -8), S(  15,  -4), S(  -3,  24), S(  -3,  41) },
        { S(  13,  -4), S(  -1,  42), S(   5,  41), S(  -4,  60) },
        { S(   5,   5), S(   4,  34), S(   1,  71), S(   0,  66) },
        { S( -13,  17), S( -47,  47), S(  -3,  63), S( -18,  88) },
        { S( -14,  29), S( -25,  40), S( -18,  59), S( -21,  65) }
    },

    // King
    {
        { S(  30,-106), S(  48, -55), S( -37, -41), S( -97, -28) },
        { S(  39, -55), S(  21, -21), S(  -8,  -7), S( -31,  -3) },
        { S( -62, -39), S(   3, -17), S( -18,   4), S( -20,  14) },
        { S(-114, -33), S( -23,   4), S( -20,  20), S( -29,  30) },
        { S( -68,  -2), S(  18,  43), S(   7,  50), S( -18,  52) },
        { S( -25,  29), S(  57,  78), S(  44,  81), S(  32,  63) },
        { S( -38,  -1), S(  17,  76), S(  43,  68), S(  37,  56) },
        { S(  26,-242), S( 105, -26), S(  77,   3), S(  17,  16) }
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
