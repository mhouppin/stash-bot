/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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
    { S(-10, 22), S( -7, 21), S(-17, 19), S( -3,  9), S( -3, 28), S( 32, 23), S( 37,  8), S( 11,-22) },
    { S( -7, 14), S(-18, 13), S(-11,  6), S( -5, -2), S(  1, 10), S(  7, 11), S( 21, -4), S( 10,-13) },
    { S( -8, 19), S(-10, 17), S(  1,-11), S(  2,-22), S( 12,-18), S( 16, -6), S( 17, -6), S(  3, -8) },
    { S( -2, 44), S(  2, 22), S(  1,  6), S( 22,-31), S( 38,-23), S( 49,-25), S( 30, -1), S( 15,  7) },
    { S( -8, 92), S(  4, 74), S( 25, 34), S( 34,-16), S( 47,-29), S(121, -0), S( 54, 20), S( 23, 38) },
    { S(109, 40), S( 97, 31), S( 93,  5), S(112,-51), S(132,-62), S( 71,-47), S(-86, 24), S(-46, 32) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -74, -65), S( -20, -31), S( -20, -10), S( -16,  11) },
        { S( -19,  -2), S( -26,  -0), S( -13,  -6), S(  -8,  14) },
        { S( -23, -28), S(  -6,   8), S( -10,  21), S(   5,  43) },
        { S(  -4,  22), S(  23,  31), S(   6,  59), S(   5,  65) },
        { S(  21,  18), S(  13,  37), S(  29,  56), S(  17,  70) },
        { S(  -5,   9), S(   9,  34), S(   7,  71), S(  15,  62) },
        { S(  -8, -20), S( -13,   5), S(  29,  -1), S(  46,  28) },
        { S(-185, -59), S(-128,  23), S(-135,  50), S(  -2,  13) }
    },

    // Bishop
    {
        { S(   9,  -3), S(   5,  11), S( -13,  15), S( -25,  21) },
        { S(   0, -18), S(   5,  -0), S(   2,   8), S( -14,  21) },
        { S(  -6,  15), S(   4,  20), S(  -3,  29), S(  -3,  38) },
        { S(  -5,  15), S(  -1,  27), S(  -2,  47), S(  17,  46) },
        { S( -16,  35), S(   8,  45), S(  16,  37), S(  30,  50) },
        { S( -13,  40), S(   9,  49), S(  25,  47), S(  23,  32) },
        { S( -41,  38), S( -30,  50), S( -18,  42), S( -29,  42) },
        { S( -75,  62), S( -59,  52), S(-170,  71), S(-136,  69) }
    },

    // Rook
    {
        { S( -31,   7), S( -26,  13), S( -23,  16), S( -18,   7) },
        { S( -75,   9), S( -33,  -2), S( -30,   5), S( -31,   4) },
        { S( -42,   8), S( -26,  26), S( -44,  27), S( -37,  23) },
        { S( -41,  36), S( -32,  48), S( -40,  52), S( -35,  44) },
        { S( -19,  56), S(  -4,  54), S(   8,  54), S(  21,  50) },
        { S( -17,  68), S(  31,  47), S(  17,  65), S(  41,  44) },
        { S(  -4,  73), S( -17,  80), S(  17,  69), S(  21,  79) },
        { S(  23,  80), S(  29,  79), S(  -6,  92), S(  -2,  85) }
    },

    // Queen
    {
        { S(  12, -51), S(  23, -77), S(  15, -65), S(  19, -35) },
        { S(  17, -59), S(  20, -63), S(  24, -59), S(  15, -25) },
        { S(  17, -42), S(  20,  -9), S(  13,  15), S(   8,  10) },
        { S(  12,  14), S(  15,  35), S(   3,  49), S(  -9,  86) },
        { S(  23,  30), S(   7,  90), S(  -0,  94), S( -11, 120) },
        { S(  15,  47), S(  24,  52), S(   1, 115), S(  -2, 118) },
        { S(  -3,  52), S( -52,  96), S( -11, 109), S( -50, 169) },
        { S(  -9,  74), S( -19, 111), S( -15, 128), S( -16, 129) }
    },

    // King
    {
        { S( 254, -33), S( 262,  53), S( 198,  77), S( 161,  67) },
        { S( 257,  52), S( 245,  93), S( 200, 126), S( 181, 133) },
        { S( 205,  81), S( 247, 107), S( 235, 137), S( 245, 154) },
        { S( 167,  95), S( 285, 130), S( 282, 159), S( 267, 176) },
        { S( 200, 117), S( 305, 160), S( 300, 178), S( 268, 185) },
        { S( 223, 123), S( 345, 171), S( 348, 179), S( 302, 162) },
        { S( 199,  67), S( 262, 169), S( 297, 141), S( 275, 124) },
        { S( 271,-164), S( 333,  16), S( 305,  42), S( 251,  70) }
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
