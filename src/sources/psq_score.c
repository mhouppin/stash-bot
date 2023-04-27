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
    { S(-11, 22), S( -7, 21), S(-18, 14), S( -2,  2), S( -3, 13), S( 36, 20), S( 36, 12), S(  5,-16) },
    { S(-10, 16), S(-21, 18), S( -1,  7), S(  1,  4), S(  6, 11), S(  4, 13), S( 19, -4), S(  6,-12) },
    { S(-12, 21), S(-12, 14), S(  5, -9), S( 12,-20), S( 20,-14), S( 17, -6), S( 13, -7), S( -6,-10) },
    { S( -2, 40), S( -7, 23), S(  2,  2), S( 24,-22), S( 39,-14), S( 47,-13), S( 18,  5), S( 10,  8) },
    { S(  6, 57), S(  6, 43), S( 28, 12), S( 46,-23), S( 51,-19), S(119,  3), S( 65, 25), S( 31, 28) },
    { S(101, 24), S( 84, 26), S( 86,  4), S(107,-34), S(120,-35), S( 67,-14), S(-61, 29), S(-46, 31) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -65, -31), S( -15, -41), S( -32, -10), S( -20,   8) },
        { S( -23, -10), S( -25,   7), S( -12,  -9), S(  -5,   6) },
        { S( -17, -20), S(  -6,   1), S(   1,   7), S(  10,  31) },
        { S(  -6,  23), S(  16,  31), S(   9,  42), S(  14,  50) },
        { S(   5,  38), S(   5,  37), S(  30,  41), S(  10,  58) },
        { S( -37,  28), S(   4,  35), S(  25,  44), S(  34,  45) },
        { S(  -9,   6), S( -27,  22), S(  40,  17), S(  40,  30) },
        { S(-175, -41), S(-123,  24), S(-117,  42), S(  14,  24) }
    },

    // Bishop
    {
        { S(  -4, -16), S(  11,   6), S( -14,  13), S( -22,  19) },
        { S(   2,   1), S(   5,  -2), S(   7,  20), S( -10,  31) },
        { S(  -1,  16), S(   7,  29), S(  -2,  22), S(   0,  54) },
        { S(  -0,   4), S(   5,  39), S(  -1,  60), S(  20,  70) },
        { S( -15,  25), S(   3,  53), S(  22,  56), S(  25,  74) },
        { S(   6,  28), S(  10,  55), S(  23,  34), S(  24,  50) },
        { S( -70,  31), S( -63,  25), S( -24,  48), S( -21,  42) },
        { S( -72,  21), S( -58,  42), S(-152,  49), S(-116,  47) }
    },

    // Rook
    {
        { S( -31,   2), S( -28,  11), S( -20,  14), S( -15,  11) },
        { S( -63,   3), S( -41,   5), S( -28,  10), S( -28,  10) },
        { S( -51,  19), S( -29,  23), S( -42,  28), S( -37,  25) },
        { S( -48,  40), S( -35,  47), S( -37,  47), S( -25,  36) },
        { S( -28,  60), S( -15,  66), S(   7,  58), S(  17,  58) },
        { S( -17,  68), S(  16,  63), S(  24,  67), S(  45,  57) },
        { S(   0,  73), S(  -7,  74), S(  36,  72), S(  47,  74) },
        { S(  12,  68), S(  22,  71), S(   6,  68), S(  14,  65) }
    },

    // Queen
    {
        { S(  11, -41), S(   7, -51), S(  14, -65), S(  20, -34) },
        { S(   9, -36), S(  17, -38), S(  22, -32), S(  19, -13) },
        { S(  12,  -9), S(  15,   9), S(  10,  32), S(   1,  33) },
        { S(   5,  33), S(  17,  39), S(  -1,  65), S(   0,  80) },
        { S(  16,  38), S(   1,  82), S(   6,  83), S(  -3, 100) },
        { S(   7,  46), S(   6,  74), S(   0, 110), S(  -0, 108) },
        { S( -11,  59), S( -48,  89), S(  -3, 105), S( -22, 130) },
        { S( -13,  70), S( -25,  83), S( -18, 101), S( -21, 107) }
    },

    // King
    {
        { S( 284,   6), S( 302,  53), S( 218,  66), S( 159,  78) },
        { S( 293,  53), S( 277,  85), S( 248,  98), S( 225, 102) },
        { S( 193,  67), S( 258,  89), S( 236, 109), S( 233, 118) },
        { S( 144,  73), S( 234, 108), S( 234, 124), S( 224, 134) },
        { S( 189, 102), S( 274, 146), S( 261, 153), S( 234, 155) },
        { S( 230, 133), S( 314, 179), S( 298, 182), S( 286, 165) },
        { S( 217, 103), S( 273, 178), S( 299, 171), S( 293, 159) },
        { S( 281,-141), S( 360,  73), S( 332, 102), S( 272, 118) }
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
