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
    { S( -9, 22), S( -8, 21), S(-14, 20), S( -5,  9), S(-10, 28), S( 32, 24), S( 37,  7), S( 11,-24) },
    { S( -9, 14), S(-17, 13), S(-10,  6), S( -5, -2), S(  4, 10), S(  9, 12), S( 16, -3), S(  8,-12) },
    { S( -7, 19), S(-10, 17), S( -2,-11), S( -1,-22), S( 13,-18), S( 15, -7), S( 18, -6), S(  6, -7) },
    { S( -2, 43), S(  1, 22), S(  1,  5), S( 21,-31), S( 42,-23), S( 51,-25), S( 31, -1), S( 14,  6) },
    { S( -8, 92), S(  4, 74), S( 25, 34), S( 34,-16), S( 47,-29), S(121,  0), S( 54, 20), S( 22, 38) },
    { S(109, 39), S( 97, 31), S( 93,  5), S(112,-51), S(132,-62), S( 71,-47), S(-86, 24), S(-46, 32) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -74, -65), S( -21, -31), S( -20, -10), S( -16,  11) },
        { S( -18,  -2), S( -26,   0), S( -12,  -6), S(  -9,  15) },
        { S( -21, -28), S(  -5,   8), S(  -9,  22), S(   5,  43) },
        { S(  -4,  22), S(  23,  31), S(   6,  59), S(   7,  66) },
        { S(  21,  18), S(  12,  37), S(  29,  56), S(  17,  70) },
        { S(  -5,   9), S(   8,  34), S(   6,  71), S(  14,  62) },
        { S(  -8, -20), S( -13,   5), S(  29,  -1), S(  46,  28) },
        { S(-185, -59), S(-128,  23), S(-135,  50), S(  -2,  13) }
    },

    // Bishop
    {
        { S(  10,  -3), S(   5,  11), S( -13,  15), S( -25,  21) },
        { S(  -0, -18), S(   3,  -0), S(   2,   8), S( -15,  21) },
        { S(  -7,  15), S(   4,  20), S(  -3,  29), S(  -2,  38) },
        { S(  -5,  15), S(  -1,  27), S(  -1,  47), S(  17,  46) },
        { S( -16,  35), S(  11,  45), S(  16,  37), S(  30,  50) },
        { S( -13,  40), S(   9,  49), S(  25,  47), S(  23,  32) },
        { S( -41,  38), S( -30,  50), S( -18,  42), S( -29,  42) },
        { S( -75,  62), S( -59,  52), S(-170,  71), S(-136,  69) }
    },

    // Rook
    {
        { S( -34,   8), S( -25,  14), S( -21,  17), S( -15,   8) },
        { S( -75,   9), S( -33,  -2), S( -30,   5), S( -31,   4) },
        { S( -43,   8), S( -26,  26), S( -44,  27), S( -37,  23) },
        { S( -41,  36), S( -32,  48), S( -41,  52), S( -35,  44) },
        { S( -19,  56), S(  -4,  53), S(   8,  54), S(  21,  50) },
        { S( -17,  68), S(  31,  46), S(  17,  65), S(  41,  44) },
        { S(  -4,  72), S( -17,  80), S(  16,  68), S(  21,  79) },
        { S(  23,  80), S(  29,  80), S(  -6,  93), S(  -2,  85) }
    },

    // Queen
    {
        { S(  12, -51), S(  24, -77), S(  16, -65), S(  20, -34) },
        { S(  17, -59), S(  21, -63), S(  24, -59), S(  17, -24) },
        { S(  17, -42), S(  21,  -9), S(  13,  15), S(   9,  10) },
        { S(  12,  14), S(  15,  35), S(   3,  49), S( -10,  86) },
        { S(  23,  30), S(   7,  90), S(  -1,  94), S( -12, 120) },
        { S(  15,  47), S(  23,  52), S(   0, 115), S(  -2, 118) },
        { S(  -3,  52), S( -53,  96), S( -12, 109), S( -50, 169) },
        { S(  -9,  74), S( -19, 111), S( -15, 128), S( -16, 129) }
    },

    // King
    {
        { S( 262, -33), S( 255,  52), S( 200,  78), S( 154,  66) },
        { S( 258,  53), S( 245,  93), S( 200, 126), S( 181, 133) },
        { S( 205,  81), S( 247, 107), S( 235, 138), S( 245, 155) },
        { S( 167,  95), S( 285, 130), S( 282, 160), S( 267, 176) },
        { S( 200, 117), S( 305, 159), S( 300, 178), S( 268, 185) },
        { S( 223, 123), S( 345, 170), S( 348, 178), S( 302, 161) },
        { S( 199,  67), S( 262, 169), S( 297, 140), S( 275, 123) },
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
