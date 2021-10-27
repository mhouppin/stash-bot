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
    { S(-13, 25), S( -7, 23), S(-15, 18), S(  4, -4), S(  5, 14), S( 34, 17), S( 45,  4), S( 15,-20) },
    { S(-12, 19), S(-21, 19), S( -9,  8), S(  3, -4), S(  5, 10), S(  8, 10), S( 26, -8), S( 14,-15) },
    { S(-16, 24), S(-17, 22), S(  1,-12), S(  5,-25), S( 13,-19), S( 15, -7), S( 13, -6), S(  1, -8) },
    { S( -9, 45), S( -6, 26), S( -0,  5), S( 25,-29), S( 29,-16), S( 43,-20), S( 25,  0), S( 13, 12) },
    { S( -6, 71), S(  3, 59), S( 16, 22), S( 31,-15), S( 50,-21), S(121, -8), S( 69, 20), S( 40, 34) },
    { S( 94, 34), S( 92, 25), S( 89,  2), S(103,-50), S(131,-55), S( 76,-28), S(-78, 41), S(-41, 49) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -67, -57), S( -19, -34), S( -21, -14), S( -21,   4) },
        { S( -26,  -8), S( -30,   1), S( -19,  -8), S( -10,   7) },
        { S( -26, -25), S( -14,  13), S( -14,  15), S(   0,  39) },
        { S(  -4,  14), S(  18,  29), S(   3,  55), S(   5,  56) },
        { S(  11,  23), S(   9,  33), S(  28,  52), S(  10,  65) },
        { S(  -7,  17), S(  18,  32), S(  22,  63), S(  28,  58) },
        { S(  -6, -11), S( -15,  15), S(  36,   9), S(  52,  25) },
        { S(-181, -54), S(-125,  28), S(-124,  56), S(   3,  18) }
    },

    // Bishop
    {
        { S(  -2,   1), S(   5,   9), S( -12,  13), S( -19,  19) },
        { S(   0, -12), S(   6,   2), S(   4,   9), S( -15,  21) },
        { S(  -2,  14), S(   3,  22), S(  -3,  30), S(  -5,  43) },
        { S(  -2,  10), S(  -1,  30), S(  -7,  49), S(  18,  51) },
        { S( -13,  25), S(  -6,  47), S(  21,  39), S(  24,  55) },
        { S(  -7,  28), S(   9,  49), S(  28,  42), S(  20,  33) },
        { S( -49,  35), S( -34,  45), S( -21,  43), S( -16,  43) },
        { S( -70,  61), S( -59,  50), S(-163,  70), S(-133,  63) }
    },

    // Rook
    {
        { S( -35,  -1), S( -29,   7), S( -23,   5), S( -21,   3) },
        { S( -61,   9), S( -41,   1), S( -33,   6), S( -35,   6) },
        { S( -41,  17), S( -30,  23), S( -46,  29), S( -45,  28) },
        { S( -46,  44), S( -37,  50), S( -38,  50), S( -36,  44) },
        { S( -29,  63), S(  -5,  65), S(   1,  60), S(  14,  53) },
        { S( -18,  74), S(  25,  60), S(  27,  63), S(  47,  45) },
        { S(  -1,  76), S(  -7,  79), S(  33,  67), S(  39,  66) },
        { S(  17,  74), S(  25,  69), S(   1,  75), S(   6,  72) }
    },

    // Queen
    {
        { S(   8, -54), S(  10, -80), S(  10, -82), S(  20, -67) },
        { S(  11, -61), S(  13, -63), S(  18, -55), S(  15, -37) },
        { S(  14, -37), S(  10, -10), S(   8,  20), S(   0,  18) },
        { S(   4,  16), S(   9,  38), S(  -2,  58), S(  -6,  87) },
        { S(  19,  34), S(  -2,  88), S(   5,  97), S(  -3, 125) },
        { S(  15,  46), S(  35,  61), S(   9, 125), S(  -1, 127) },
        { S(  -1,  55), S( -44, 100), S(  -4, 117), S( -36, 168) },
        { S(  -7,  76), S( -16, 109), S( -11, 127), S( -12, 129) }
    },

    // King
    {
        { S( 257,  -4), S( 285,  45), S( 210,  75), S( 166,  82) },
        { S( 271,  50), S( 267,  85), S( 231, 108), S( 196, 119) },
        { S( 200,  72), S( 246,  96), S( 223, 121), S( 226, 134) },
        { S( 161,  89), S( 259, 112), S( 259, 133), S( 239, 148) },
        { S( 201, 124), S( 293, 152), S( 280, 157), S( 244, 166) },
        { S( 230, 138), S( 341, 181), S( 335, 183), S( 297, 170) },
        { S( 207,  83), S( 270, 190), S( 306, 172), S( 286, 152) },
        { S( 273,-161), S( 341,  31), S( 313,  58), S( 260,  86) }
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
