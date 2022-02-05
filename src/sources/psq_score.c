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

#include "imath.h"
#include "psq_score.h"

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
    { S(-10, 23), S( -7, 22), S(-17, 22), S( -0,  1), S(  1, 17), S( 33, 16), S( 39,  0), S( 12,-17) },
    { S(-10, 16), S(-21, 24), S( -5, 14), S( -4,  5), S(  6, 13), S(  4, 16), S( 23,-13), S( 12,-12) },
    { S(-11, 25), S(-11, 17), S(  3, -4), S(  8,-20), S( 13,-13), S( 19, -7), S( 18, -9), S(  4, -6) },
    { S( -3, 44), S( -5, 26), S(  3,  8), S( 22,-20), S( 36,-17), S( 47,-20), S( 27, -2), S( 10, 12) },
    { S(  6, 64), S(  9, 44), S( 17, 22), S( 22,-15), S( 45,-29), S(122, -8), S( 71, 12), S( 34, 29) },
    { S( 89, 23), S( 85, 21), S( 90,  1), S(100,-54), S(125,-58), S( 73,-22), S(-76, 44), S(-45, 50) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -50), S( -21, -30), S( -27, -13), S( -20,   8) },
        { S( -28,  -8), S( -27,   9), S( -10,  -9), S(  -6,   7) },
        { S( -22, -24), S(  -4,   2), S(  -4,   9), S(   4,  36) },
        { S(  -7,  18), S(  14,  32), S(   8,  43), S(   9,  53) },
        { S(   5,  29), S(   2,  35), S(  23,  47), S(   7,  69) },
        { S( -15,  18), S(   9,  32), S(  29,  47), S(  22,  51) },
        { S(  -5,  -3), S( -15,  21), S(  34,  12), S(  49,  26) },
        { S(-177, -48), S(-123,  32), S(-118,  60), S(   6,  18) }
    },

    // Bishop
    {
        { S(   3,  -2), S(  -2,   7), S( -18,   9), S( -25,  22) },
        { S(   2,  -5), S(   4,   1), S(  -3,  10), S( -11,  23) },
        { S(  -4,  15), S(   4,  21), S(  -2,  34), S(  -5,  43) },
        { S(  -1,   5), S(  -2,  31), S(  -6,  49), S(  14,  56) },
        { S(  -6,  23), S(  -3,  47), S(  19,  39), S(  16,  63) },
        { S(   4,  25), S(   6,  51), S(  24,  41), S(  17,  36) },
        { S( -41,  40), S( -32,  44), S( -21,  45), S( -16,  39) },
        { S( -66,  59), S( -59,  48), S(-157,  66), S(-125,  61) }
    },

    // Rook
    {
        { S( -28,   3), S( -25,  10), S( -22,  15), S( -18,   8) },
        { S( -51,  10), S( -39,   7), S( -31,   7), S( -31,  12) },
        { S( -43,  20), S( -33,  21), S( -43,  30), S( -40,  29) },
        { S( -51,  48), S( -41,  51), S( -36,  53), S( -27,  41) },
        { S( -23,  60), S(  -7,  71), S(  -3,  64), S(  13,  52) },
        { S( -16,  70), S(  22,  57), S(  22,  68), S(  47,  47) },
        { S(  -4,  74), S( -10,  80), S(  27,  67), S(  33,  62) },
        { S(  16,  61), S(  21,  64), S(   4,  68), S(  11,  63) }
    },

    // Queen
    {
        { S(  19, -54), S(  16, -72), S(  18, -83), S(  21, -55) },
        { S(  18, -51), S(  24, -56), S(  23, -48), S(  23, -40) },
        { S(  11, -29), S(  14,  -1), S(   8,  22), S(   3,  23) },
        { S(   6,  21), S(  10,  43), S(  -2,  60), S(   3,  78) },
        { S(  12,  38), S(  -4,  86), S(  -2,  97), S(  -7, 115) },
        { S(   2,  46), S(  20,  63), S(  -1, 120), S(  -5, 124) },
        { S(  -6,  59), S( -46, 105), S(  -2, 116), S( -36, 155) },
        { S(  -9,  74), S( -23,  98), S( -15, 116), S( -25, 112) }
    },

    // King
    {
        { S( 269,   0), S( 295,  49), S( 222,  80), S( 168,  97) },
        { S( 276,  50), S( 275,  86), S( 245, 106), S( 211, 116) },
        { S( 194,  84), S( 251,  96), S( 232, 121), S( 223, 130) },
        { S( 157,  91), S( 249, 113), S( 245, 128), S( 226, 145) },
        { S( 199, 117), S( 280, 140), S( 266, 150), S( 231, 162) },
        { S( 231, 140), S( 330, 174), S( 312, 169), S( 289, 161) },
        { S( 214,  95), S( 273, 183), S( 307, 164), S( 291, 152) },
        { S( 276,-158), S( 351,  41), S( 322,  69), S( 267,  96) }
    }
};

#undef S

void psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t pieceValue = create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

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
