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
    { S(-10, 21), S(-13, 14), S(-16, 18), S( -4,  8), S(-11, 25), S( 32, 22), S( 31, -1), S(  8,-25) },
    { S( -8, 14), S(-14, 10), S( -4,  4), S(  0, -4), S( 10,  9), S( 15,  9), S( 20, -6), S(  9,-12) },
    { S(-11, 21), S( -8, 16), S( -1, -9), S(  3,-20), S( 12,-15), S( 17, -5), S( 15, -6), S(  1, -6) },
    { S( -5, 45), S(  3, 27), S(  3,  9), S( 22,-26), S( 43,-18), S( 52,-18), S( 29,  5), S(  5,  7) },
    { S( -6, 97), S( 17, 82), S( 34, 46), S( 34, -2), S( 47,-15), S(135, 13), S( 60, 34), S( 20, 43) },
    { S(105, 40), S( 95, 34), S( 91,  7), S(110,-48), S(127,-61), S( 78,-44), S(-68, 17), S(-32, 21) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -70, -67), S( -17, -32), S( -15, -12), S( -12,  10) },
        { S( -15,  -5), S( -24,  -1), S( -10, -10), S(  -6,  13) },
        { S( -19, -31), S(  -2,   5), S(  -7,  16), S(  10,  41) },
        { S(  -1,  21), S(  23,  29), S(   9,  56), S(  10,  63) },
        { S(  22,  18), S(  11,  36), S(  29,  56), S(  17,  72) },
        { S(   2,   7), S(   6,  32), S(   7,  68), S(  13,  61) },
        { S(  -2, -24), S(  -7,   4), S(  36,  -4), S(  49,  29) },
        { S(-189, -48), S(-128,  25), S(-132,  52), S(   1,  16) }
    },

    // Bishop
    {
        { S(  13,  -2), S(   7,  10), S( -12,  13), S( -25,  21) },
        { S(   3, -16), S(   4,  -2), S(   4,   6), S( -13,  19) },
        { S(  -3,  12), S(   6,  18), S(   0,  27), S(   1,  36) },
        { S(  -3,  12), S(   2,  24), S(  -1,  45), S(  19,  44) },
        { S( -15,  34), S(  12,  42), S(  19,  35), S(  33,  49) },
        { S(  -8,  38), S(  13,  44), S(  30,  44), S(  26,  30) },
        { S( -35,  35), S( -29,  48), S( -17,  41), S( -34,  42) },
        { S( -72,  60), S( -59,  52), S(-160,  68), S(-134,  67) }
    },

    // Rook
    {
        { S( -32,   6), S( -24,  12), S( -19,  15), S( -14,   5) },
        { S( -74,   8), S( -32,  -3), S( -29,   4), S( -30,   4) },
        { S( -42,   7), S( -26,  25), S( -42,  25), S( -35,  22) },
        { S( -40,  35), S( -32,  48), S( -42,  52), S( -35,  44) },
        { S( -18,  55), S(  -2,  51), S(   8,  53), S(  23,  48) },
        { S( -15,  65), S(  35,  42), S(  22,  61), S(  47,  41) },
        { S(  -1,  70), S( -16,  78), S(  20,  66), S(  25,  76) },
        { S(  26,  79), S(  28,  81), S(  -8,  94), S(  -3,  87) }
    },

    // Queen
    {
        { S(  17, -59), S(  27, -83), S(  21, -69), S(  26, -37) },
        { S(  22, -64), S(  26, -69), S(  30, -64), S(  24, -29) },
        { S(  18, -42), S(  25, -12), S(  16,  12), S(  12,  10) },
        { S(  13,   8), S(  17,  33), S(   0,  49), S(  -8,  81) },
        { S(  13,  32), S(   7,  86), S(  -4,  89), S( -16, 116) },
        { S( -12,  60), S(  13,  53), S(  -3, 106), S( -10, 115) },
        { S(  -4,  50), S( -50,  91), S( -13, 100), S( -68, 169) },
        { S( -18,  69), S( -19, 102), S( -19, 120), S( -35, 122) }
    },

    // King
    {
        { S( 260, -31), S( 255,  54), S( 198,  79), S( 156,  65) },
        { S( 256,  56), S( 245,  95), S( 202, 127), S( 184, 131) },
        { S( 200,  85), S( 257, 105), S( 240, 136), S( 251, 152) },
        { S( 166,  98), S( 292, 126), S( 285, 156), S( 267, 173) },
        { S( 203, 117), S( 306, 153), S( 300, 172), S( 270, 179) },
        { S( 225, 122), S( 339, 171), S( 342, 174), S( 303, 157) },
        { S( 193,  73), S( 265, 171), S( 290, 142), S( 260, 131) },
        { S( 239,-144), S( 320,  34), S( 277,  54), S( 234,  82) }
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
