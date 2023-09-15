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
    { S(-33, 17), S(-30, 16), S(-41,  7), S(-24, -3), S(-25,  6), S( 13, 15), S( 15,  7), S(-17,-22) },
    { S(-31, 10), S(-44, 13), S(-23,  1), S(-20, -3), S(-16,  5), S(-19,  7), S( -3,-11), S(-16,-19) },
    { S(-33, 16), S(-34,  9), S(-16,-16), S(-10,-28), S( -2,-21), S( -3,-13), S( -9,-13), S(-26,-17) },
    { S(-22, 36), S(-27, 19), S(-18, -4), S(  4,-29), S( 19,-20), S( 26,-19), S( -3, -1), S(-11,  2) },
    { S(-15, 54), S(-15, 41), S(  8,  7), S( 25,-29), S( 32,-25), S(102, -3), S( 46, 21), S( 11, 22) },
    { S( 83, 21), S( 64, 22), S( 64,  0), S( 87,-40), S( 97,-39), S( 43,-19), S(-76, 23), S(-66, 25) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -44), S(  -2, -58), S( -17, -27), S(  -5,  -8) },
        { S(  -9, -27), S( -11, -10), S(   4, -27), S(   8, -11) },
        { S(  -2, -37), S(   8, -16), S(  14, -10), S(  25,  15) },
        { S(   8,   7), S(  31,  15), S(  24,  26), S(  27,  35) },
        { S(  20,  22), S(  19,  21), S(  44,  26), S(  24,  43) },
        { S( -25,  13), S(  18,  19), S(  39,  27), S(  48,  29) },
        { S(   6, -11), S( -15,   7), S(  55,   0), S(  54,  15) },
        { S(-163, -58), S(-110,   6), S(-102,  25), S(  30,   8) }
    },

    // Bishop
    {
        { S(  13, -51), S(  27, -28), S(  -1, -19), S(  -7, -13) },
        { S(  18, -34), S(  20, -35), S(  23, -13), S(   4,  -1) },
        { S(  14, -17), S(  23,  -3), S(  13, -11), S(  15,  23) },
        { S(  16, -30), S(  20,   7), S(  14,  29), S(  35,  40) },
        { S(   1,  -7), S(  18,  22), S(  37,  25), S(  40,  45) },
        { S(  21,  -4), S(  26,  25), S(  39,   2), S(  39,  18) },
        { S( -57,  -0), S( -49,  -6), S(  -9,  18), S(  -6,  11) },
        { S( -59, -16), S( -43,  10), S(-137,  18), S( -99,  16) }
    },

    // Rook
    {
        { S( -20, -43), S( -16, -34), S(  -8, -30), S(  -3, -34) },
        { S( -51, -41), S( -28, -39), S( -16, -34), S( -16, -35) },
        { S( -39, -25), S( -18, -21), S( -29, -15), S( -25, -18) },
        { S( -37,  -2), S( -23,   5), S( -25,   5), S( -13,  -7) },
        { S( -16,  19), S(  -3,  26), S(  19,  16), S(  29,  17) },
        { S(  -4,  28), S(  28,  23), S(  37,  26), S(  58,  15) },
        { S(  13,  32), S(   6,  34), S(  49,  32), S(  61,  33) },
        { S(  24,  27), S(  34,  30), S(  17,  28), S(  25,  24) }
    },

    // Queen
    {
        { S(   8, -80), S(   4, -91), S(  11,-107), S(  15, -71) },
        { S(   6, -75), S(  15, -78), S(  19, -72), S(  16, -53) },
        { S(   9, -46), S(  14, -31), S(   8,  -7), S(  -2,  -5) },
        { S(   4,  -6), S(  15,  -2), S(  -2,  27), S(  -3,  44) },
        { S(  14,  -2), S(  -1,  45), S(   5,  43), S(  -4,  63) },
        { S(   6,   7), S(   5,  37), S(   2,  73), S(   1,  68) },
        { S( -12,  19), S( -46,  49), S(  -2,  66), S( -16,  89) },
        { S( -12,  30), S( -24,  41), S( -17,  60), S( -19,  66) }
    },

    // King
    {
        { S(  30,-105), S(  47, -55), S( -37, -41), S( -97, -28) },
        { S(  38, -55), S(  21, -21), S(  -8,  -8), S( -31,  -3) },
        { S( -62, -39), S(   3, -17), S( -17,   4), S( -19,  13) },
        { S(-114, -33), S( -23,   4), S( -20,  20), S( -28,  30) },
        { S( -68,  -2), S(  18,  43), S(   8,  50), S( -17,  52) },
        { S( -25,  29), S(  57,  78), S(  44,  81), S(  33,  62) },
        { S( -39,  -1), S(  16,  76), S(  43,  68), S(  37,  56) },
        { S(  27,-240), S( 106, -24), S(  77,   4), S(  16,  17) }
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
