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
    { S(-10, 23), S( -6, 21), S(-19, 19), S( -2,  8), S( -1, 27), S( 31, 21), S( 39,  9), S( 12,-20) },
    { S( -7, 14), S(-18, 13), S(-12,  6), S( -3, -3), S(  1, 11), S(  6, 11), S( 23, -5), S( 12,-14) },
    { S( -8, 19), S(-11, 17), S(  1,-12), S(  3,-23), S( 12,-18), S( 17, -6), S( 15, -6), S(  1, -9) },
    { S( -1, 45), S(  2, 22), S(  1,  6), S( 23,-31), S( 34,-23), S( 47,-25), S( 29, -1), S( 15,  8) },
    { S( -7, 91), S(  4, 74), S( 24, 33), S( 34,-16), S( 47,-29), S(121, -1), S( 54, 20), S( 24, 38) },
    { S(109, 41), S( 97, 32), S( 93,  5), S(112,-51), S(132,-62), S( 71,-46), S(-86, 25), S(-46, 33) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -74, -65), S( -19, -31), S( -20, -10), S( -17,  11) },
        { S( -20,  -2), S( -26,  -0), S( -14,  -6), S(  -8,  13) },
        { S( -24, -28), S(  -8,   8), S( -11,  21), S(   5,  43) },
        { S(  -3,  22), S(  23,  31), S(   6,  59), S(   5,  64) },
        { S(  21,  18), S(  14,  37), S(  29,  56), S(  15,  69) },
        { S(  -5,   9), S(  10,  34), S(   8,  71), S(  16,  63) },
        { S(  -8, -20), S( -13,   5), S(  29,  -0), S(  46,  28) },
        { S(-185, -59), S(-128,  23), S(-135,  50), S(  -2,  13) }
    },

    // Bishop
    {
        { S(   7,  -3), S(   5,  11), S( -12,  15), S( -24,  21) },
        { S(   0, -18), S(   7,  -0), S(   3,   8), S( -14,  21) },
        { S(  -5,  15), S(   4,  20), S(  -3,  29), S(  -3,  38) },
        { S(  -5,  15), S(  -1,  27), S(  -4,  47), S(  18,  46) },
        { S( -16,  35), S(   3,  44), S(  17,  37), S(  30,  50) },
        { S( -12,  40), S(   9,  49), S(  25,  47), S(  23,  32) },
        { S( -41,  38), S( -30,  50), S( -18,  42), S( -29,  42) },
        { S( -75,  62), S( -59,  52), S(-170,  71), S(-136,  69) }
    },

    // Rook
    {
        { S( -33,   5), S( -27,  12), S( -23,  14), S( -20,   6) },
        { S( -74,   9), S( -34,  -2), S( -31,   5), S( -31,   4) },
        { S( -41,   9), S( -26,  26), S( -44,  27), S( -38,  23) },
        { S( -41,  37), S( -32,  48), S( -40,  52), S( -35,  44) },
        { S( -19,  56), S(  -3,  55), S(   8,  54), S(  21,  50) },
        { S( -17,  69), S(  31,  48), S(  18,  65), S(  41,  44) },
        { S(  -3,  74), S( -16,  81), S(  18,  70), S(  22,  79) },
        { S(  23,  79), S(  29,  78), S(  -6,  91), S(  -2,  84) }
    },

    // Queen
    {
        { S(  12, -51), S(  22, -77), S(  14, -66), S(  20, -37) },
        { S(  17, -59), S(  19, -63), S(  22, -59), S(  16, -26) },
        { S(  17, -42), S(  17,  -9), S(  13,  15), S(   7,  10) },
        { S(  11,  14), S(  15,  35), S(   3,  49), S(  -8,  87) },
        { S(  23,  30), S(   6,  90), S(   1,  94), S( -10, 121) },
        { S(  15,  47), S(  25,  53), S(   2, 116), S(  -2, 118) },
        { S(  -3,  52), S( -51,  96), S( -10, 109), S( -49, 169) },
        { S(  -9,  74), S( -19, 111), S( -15, 128), S( -16, 129) }
    },

    // King
    {
        { S( 249, -32), S( 267,  50), S( 196,  76), S( 165,  68) },
        { S( 256,  52), S( 247,  93), S( 201, 125), S( 181, 132) },
        { S( 204,  80), S( 246, 106), S( 234, 136), S( 244, 153) },
        { S( 167,  95), S( 284, 129), S( 281, 158), S( 266, 175) },
        { S( 200, 118), S( 305, 161), S( 300, 178), S( 268, 185) },
        { S( 223, 124), S( 345, 172), S( 348, 180), S( 302, 163) },
        { S( 199,  67), S( 262, 170), S( 297, 142), S( 275, 125) },
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
