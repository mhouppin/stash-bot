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
    { S(-14, 27), S( -7, 21), S(-15, 18), S(  2, -4), S(  4, 12), S( 32, 15), S( 41,  2), S( 12,-20) },
    { S(-13, 21), S(-22, 20), S( -7,  8), S(  4, -4), S(  4, 10), S(  7, 11), S( 23, -8), S( 12,-14) },
    { S(-16, 25), S(-16, 22), S(  2,-11), S(  7,-24), S( 14,-18), S( 16, -7), S( 11, -5), S( -0, -6) },
    { S( -9, 46), S( -6, 27), S(  2,  5), S( 25,-26), S( 31,-16), S( 44,-20), S( 26,  1), S( 12, 12) },
    { S( -3, 70), S(  6, 55), S( 17, 20), S( 29,-16), S( 51,-24), S(124,-10), S( 73, 19), S( 41, 34) },
    { S( 90, 30), S( 91, 22), S( 89,  1), S(101,-50), S(131,-54), S( 78,-22), S(-73, 46), S(-41, 50) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -64, -53), S( -20, -37), S( -24, -16), S( -24,   8) },
        { S( -27, -12), S( -33,   1), S( -19,  -8), S( -10,   9) },
        { S( -25, -23), S( -14,  11), S(  -9,  13), S(   3,  37) },
        { S(  -6,  16), S(  18,  30), S(   3,  54), S(   4,  55) },
        { S(   9,  25), S(   8,  31), S(  27,  49), S(   9,  65) },
        { S(  -9,  18), S(  18,  31), S(  24,  62), S(  32,  52) },
        { S(  -8,  -8), S( -16,  19), S(  36,  11), S(  55,  25) },
        { S(-180, -52), S(-124,  30), S(-120,  58), S(   5,  18) }
    },

    // Bishop
    {
        { S(  -4,   3), S(   2,   9), S( -14,  10), S( -23,  20) },
        { S(   5,  -8), S(   6,   1), S(   5,   9), S( -15,  22) },
        { S(  -3,  14), S(   4,  25), S(  -2,  32), S(  -3,  42) },
        { S(  -3,   9), S(  -2,  28), S(  -7,  50), S(  16,  55) },
        { S( -12,  24), S(  -5,  46), S(  19,  40), S(  23,  58) },
        { S(  -5,  27), S(  10,  50), S(  27,  40), S(  19,  33) },
        { S( -50,  33), S( -34,  45), S( -18,  46), S( -15,  40) },
        { S( -70,  59), S( -59,  49), S(-159,  69), S(-130,  63) }
    },

    // Rook
    {
        { S( -34,   1), S( -29,   9), S( -24,   9), S( -21,   5) },
        { S( -62,   9), S( -42,   4), S( -33,   7), S( -36,   8) },
        { S( -43,  20), S( -31,  24), S( -47,  29), S( -46,  29) },
        { S( -46,  46), S( -39,  51), S( -37,  50), S( -38,  45) },
        { S( -28,  64), S(  -6,  66), S(   1,  61), S(  12,  52) },
        { S( -18,  73), S(  26,  59), S(  29,  62), S(  47,  43) },
        { S(  -1,  75), S(  -6,  77), S(  33,  66), S(  46,  65) },
        { S(  16,  72), S(  24,  66), S(   3,  73), S(  11,  70) }
    },

    // Queen
    {
        { S(  15, -54), S(  14, -79), S(  14, -84), S(  22, -64) },
        { S(  12, -59), S(  15, -60), S(  22, -52), S(  18, -33) },
        { S(  11, -32), S(  13,  -5), S(  10,  23), S(   2,  23) },
        { S(   5,  18), S(   9,  40), S(  -4,  60), S(  -3,  83) },
        { S(  15,  37), S(  -1,  88), S(   0,  94), S(  -9, 116) },
        { S(   9,  49), S(  31,  63), S(   4, 121), S(  -7, 123) },
        { S(  -3,  56), S( -46, 100), S(  -4, 117), S( -39, 159) },
        { S(  -6,  77), S( -17, 106), S( -12, 122), S( -17, 121) }
    },

    // King
    {
        { S( 260,  -9), S( 289,  40), S( 218,  74), S( 166,  86) },
        { S( 268,  43), S( 268,  80), S( 233, 107), S( 203, 117) },
        { S( 194,  67), S( 251,  91), S( 229, 119), S( 228, 132) },
        { S( 158,  87), S( 253, 112), S( 253, 134), S( 232, 147) },
        { S( 200, 120), S( 288, 150), S( 274, 157), S( 236, 165) },
        { S( 231, 140), S( 337, 179), S( 329, 183), S( 294, 169) },
        { S( 210,  88), S( 272, 194), S( 308, 178), S( 290, 160) },
        { S( 274,-159), S( 345,  38), S( 316,  65), S( 264,  93) }
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
