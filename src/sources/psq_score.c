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
    { S( -9, 20), S( -6, 20), S(-14, 14), S( -1,  2), S(  0, 13), S( 34, 21), S( 35, 13), S(  7,-15) },
    { S( -7, 16), S(-17, 18), S(  1,  8), S(  2,  5), S(  8, 10), S(  7, 14), S( 21, -3), S(  8,-11) },
    { S(-10, 20), S( -9, 13), S(  5, -8), S( 11,-18), S( 19,-13), S( 17, -5), S( 15, -5), S( -3, -9) },
    { S( -1, 38), S( -6, 21), S(  2,  2), S( 22,-20), S( 37,-13), S( 43,-11), S( 16,  5), S(  9,  9) },
    { S(  3, 52), S(  5, 39), S( 25, 10), S( 39,-23), S( 48,-20), S(111,  4), S( 65, 22), S( 29, 26) },
    { S( 96, 24), S( 82, 23), S( 87,  1), S(105,-36), S(122,-39), S( 70,-15), S(-66, 31), S(-46, 34) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -64, -36), S( -13, -38), S( -29,  -9), S( -18,   8) },
        { S( -21, -11), S( -24,   7), S( -12,  -8), S(  -6,   7) },
        { S( -15, -19), S(  -6,   2), S(  -1,   6), S(   8,  31) },
        { S(  -7,  23), S(  13,  31), S(   7,  41), S(  11,  48) },
        { S(   3,  36), S(   2,  37), S(  26,  41), S(   8,  56) },
        { S( -33,  24), S(   3,  34), S(  22,  43), S(  32,  45) },
        { S( -11,   5), S( -24,  19), S(  36,  17), S(  40,  30) },
        { S(-174, -42), S(-122,  27), S(-117,  45), S(  12,  25) }
    },

    // Bishop
    {
        { S(  -4, -10), S(   8,   7), S( -13,  14), S( -21,  20) },
        { S(   1,   1), S(   4,   1), S(   5,  19), S( -10,  28) },
        { S(  -1,  16), S(   4,  29), S(  -3,  23), S(  -0,  51) },
        { S(  -2,   4), S(   3,  37), S(  -1,  56), S(  18,  65) },
        { S( -15,  24), S(   3,  50), S(  18,  51), S(  21,  69) },
        { S(   3,  28), S(   7,  52), S(  19,  34), S(  20,  46) },
        { S( -63,  30), S( -57,  27), S( -23,  45), S( -21,  41) },
        { S( -69,  34), S( -58,  44), S(-151,  50), S(-117,  48) }
    },

    // Rook
    {
        { S( -30,   3), S( -27,  11), S( -20,  14), S( -16,  12) },
        { S( -57,   5), S( -39,   7), S( -28,  11), S( -29,  12) },
        { S( -50,  19), S( -30,  24), S( -41,  28), S( -36,  26) },
        { S( -47,  40), S( -34,  46), S( -36,  46), S( -26,  36) },
        { S( -28,  58), S( -16,  64), S(   2,  55), S(  12,  57) },
        { S( -18,  67), S(  13,  61), S(  21,  66), S(  41,  54) },
        { S(  -4,  71), S( -10,  72), S(  31,  70), S(  40,  72) },
        { S(  11,  64), S(  21,  69), S(   7,  67), S(  15,  63) }
    },

    // Queen
    {
        { S(  15, -44), S(  12, -54), S(  16, -65), S(  20, -33) },
        { S(  11, -41), S(  18, -40), S(  22, -31), S(  19, -13) },
        { S(  15, -15), S(  15,   7), S(  10,  29), S(   3,  29) },
        { S(   6,  29), S(  16,  38), S(   1,  61), S(   1,  73) },
        { S(  14,  37), S(  -0,  78), S(   2,  83), S(  -5,  98) },
        { S(   6,  43), S(   4,  68), S(  -4, 108), S(  -5, 109) },
        { S( -12,  58), S( -47,  90), S(  -6, 104), S( -30, 132) },
        { S( -14,  70), S( -27,  85), S( -20, 101), S( -25, 107) }
    },

    // King
    {
        { S( 282,  11), S( 298,  57), S( 224,  69), S( 165,  80) },
        { S( 290,  57), S( 274,  87), S( 246,  98), S( 226, 103) },
        { S( 193,  72), S( 256,  91), S( 233, 110), S( 228, 118) },
        { S( 149,  77), S( 238, 109), S( 235, 124), S( 222, 133) },
        { S( 192, 105), S( 275, 145), S( 260, 152), S( 230, 154) },
        { S( 231, 135), S( 317, 176), S( 298, 180), S( 285, 165) },
        { S( 217, 104), S( 275, 177), S( 301, 171), S( 294, 157) },
        { S( 280,-146), S( 359,  63), S( 332,  93), S( 272, 113) }
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
