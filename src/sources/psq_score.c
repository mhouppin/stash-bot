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
    { S(-33, 19), S(-32, 15), S(-46,  6), S(-25, -4), S(-28,  5), S( 14, 14), S( 16,  7), S(-18,-23) },
    { S(-31, 10), S(-46, 12), S(-24, -1), S(-21, -4), S(-16,  4), S(-22,  6), S( -5,-12), S(-16,-21) },
    { S(-33, 17), S(-35, 10), S(-16,-17), S( -9,-30), S( -1,-23), S( -4,-14), S(-10,-13), S(-27,-18) },
    { S(-21, 39), S(-25, 21), S(-17, -3), S(  7,-30), S( 24,-21), S( 29,-19), S( -2,  0), S(-11,  4) },
    { S(-12, 57), S(-12, 43), S(  9,  8), S( 27,-28), S( 32,-25), S(102, -0), S( 45, 21), S( 12, 24) },
    { S( 83, 24), S( 65, 24), S( 66,  1), S( 87,-38), S( 99,-39), S( 46,-19), S(-80, 24), S(-67, 26) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -48), S(  -4, -60), S( -20, -29), S(  -7, -10) },
        { S( -11, -28), S( -12, -11), S(   4, -27), S(   8, -11) },
        { S(  -4, -38), S(   7, -16), S(  14,  -9), S(  28,  17) },
        { S(   8,   6), S(  33,  15), S(  26,  27), S(  30,  36) },
        { S(  21,  22), S(  20,  21), S(  48,  27), S(  26,  45) },
        { S( -24,  12), S(  19,  19), S(  40,  28), S(  49,  29) },
        { S(   5, -11), S( -13,   6), S(  55,   1), S(  55,  16) },
        { S(-161, -58), S(-109,   7), S(-103,  26), S(  29,   8) }
    },

    // Bishop
    {
        { S(  10, -51), S(  27, -28), S(  -4, -21), S(  -9, -16) },
        { S(  18, -33), S(  21, -38), S(  25, -14), S(   3,  -2) },
        { S(  14, -18), S(  25,  -4), S(  13, -12), S(  15,  25) },
        { S(  16, -30), S(  21,   7), S(  14,  30), S(  38,  42) },
        { S(   0,  -7), S(  18,  23), S(  39,  27), S(  44,  47) },
        { S(  22,  -4), S(  27,  25), S(  40,   3), S(  41,  19) },
        { S( -57,  -2), S( -49,  -8), S(  -9,  17), S(  -6,  10) },
        { S( -58, -13), S( -43,   9), S(-137,  17), S(-101,  15) }
    },

    // Rook
    {
        { S( -21, -44), S( -16, -36), S(  -7, -32), S(  -1, -37) },
        { S( -54, -44), S( -30, -41), S( -16, -37), S( -17, -39) },
        { S( -41, -27), S( -18, -22), S( -31, -17), S( -26, -21) },
        { S( -37,  -3), S( -23,   5), S( -25,   5), S( -13,  -8) },
        { S( -14,  20), S(  -2,  28), S(  22,  18), S(  32,  18) },
        { S(  -3,  30), S(  30,  25), S(  39,  29), S(  60,  17) },
        { S(  15,  35), S(   7,  37), S(  51,  34), S(  62,  35) },
        { S(  25,  27), S(  35,  31), S(  18,  28), S(  26,  24) }
    },

    // Queen
    {
        { S(   7, -83), S(   2, -94), S(   9,-110), S(  15, -76) },
        { S(   6, -78), S(  15, -80), S(  20, -76), S(  16, -57) },
        { S(  10, -50), S(  14, -33), S(   8,  -8), S(  -3,  -7) },
        { S(   4,  -7), S(  16,  -2), S(  -2,  26), S(  -2,  44) },
        { S(  15,  -2), S(   1,  43), S(   6,  44), S(  -2,  63) },
        { S(   7,   6), S(   6,  35), S(   2,  73), S(   0,  69) },
        { S( -12,  18), S( -48,  49), S(  -2,  66), S( -20,  91) },
        { S( -14,  29), S( -26,  41), S( -19,  60), S( -22,  67) }
    },

    // King
    {
        { S(  28,-106), S(  49, -57), S( -42, -43), S(-100, -30) },
        { S(  39, -56), S(  22, -21), S(  -6,  -7), S( -30,  -3) },
        { S( -63, -41), S(   4, -17), S( -17,   5), S( -20,  16) },
        { S(-112, -33), S( -21,   5), S( -20,  22), S( -30,  33) },
        { S( -66,  -1), S(  19,  45), S(   7,  53), S( -20,  55) },
        { S( -25,  30), S(  59,  79), S(  43,  82), S(  32,  64) },
        { S( -38,  -1), S(  18,  75), S(  44,  68), S(  38,  56) },
        { S(  26,-244), S( 105, -29), S(  77,  -0), S(  17,  15) }
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
