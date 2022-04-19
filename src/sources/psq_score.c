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
    { S(-11, 25), S( -6, 20), S(-16, 19), S( -0,  4), S(  1, 18), S( 31, 18), S( 39,  3), S( 10,-16) },
    { S(-10, 16), S(-20, 24), S( -3, 11), S( -1,  6), S(  6, 14), S(  3, 16), S( 23, -9), S( 13,-12) },
    { S(-10, 25), S(-11, 16), S(  5, -9), S(  9,-19), S( 13,-14), S( 18, -6), S( 17, -9), S(  4, -6) },
    { S( -3, 44), S( -4, 26), S(  2,  8), S( 23,-22), S( 35,-15), S( 48,-18), S( 23,  1), S( 11, 11) },
    { S(  6, 64), S(  7, 42), S( 18, 20), S( 27,-15), S( 45,-29), S(118,-10), S( 71, 11), S( 36, 27) },
    { S( 91, 24), S( 84, 20), S( 90,  0), S(101,-54), S(125,-57), S( 73,-21), S(-76, 43), S(-46, 49) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -48), S( -17, -33), S( -25, -12), S( -19,  11) },
        { S( -22,  -7), S( -26,  12), S( -12, -10), S(  -6,   5) },
        { S( -20, -25), S(  -7,   1), S(  -5,   8), S(   5,  33) },
        { S(  -6,  20), S(  11,  31), S(   8,  46), S(   9,  52) },
        { S(   5,  30), S(   2,  34), S(  22,  46), S(   5,  68) },
        { S( -15,  19), S(   4,  30), S(  29,  45), S(  24,  52) },
        { S(  -9,  -2), S( -15,  21), S(  32,  11), S(  48,  27) },
        { S(-176, -46), S(-123,  33), S(-117,  61), S(   6,  19) }
    },

    // Bishop
    {
        { S(   4,  -3), S(  -1,   6), S( -17,  10), S( -23,  24) },
        { S(   1,  -3), S(   4,   1), S(   2,   9), S( -13,  25) },
        { S(  -5,  15), S(   7,  22), S(  -3,  35), S(  -4,  42) },
        { S(  -2,   5), S(  -4,  31), S(  -5,  51), S(  13,  57) },
        { S( -13,  21), S(  -3,  46), S(  18,  39), S(  19,  63) },
        { S(   1,  24), S(   7,  48), S(  21,  39), S(  16,  35) },
        { S( -41,  41), S( -34,  43), S( -20,  44), S( -17,  40) },
        { S( -65,  59), S( -59,  48), S(-156,  67), S(-123,  62) }
    },

    // Rook
    {
        { S( -29,   3), S( -26,  10), S( -21,  12), S( -18,   9) },
        { S( -52,   9), S( -40,   8), S( -31,   8), S( -29,  13) },
        { S( -43,  20), S( -32,  23), S( -40,  30), S( -41,  28) },
        { S( -51,  45), S( -42,  52), S( -39,  51), S( -25,  42) },
        { S( -25,  59), S(  -8,  70), S(  -2,  64), S(  16,  54) },
        { S( -17,  68), S(  19,  57), S(  21,  66), S(  46,  46) },
        { S(  -4,  74), S( -10,  80), S(  28,  69), S(  35,  66) },
        { S(  16,  62), S(  19,  62), S(   5,  68), S(  11,  62) }
    },

    // Queen
    {
        { S(  17, -53), S(  15, -71), S(  14, -82), S(  21, -54) },
        { S(  14, -51), S(  23, -56), S(  22, -46), S(  23, -38) },
        { S(  13, -27), S(  15,  -2), S(   9,  23), S(   4,  21) },
        { S(   7,  23), S(  11,  43), S(  -1,  60), S(  -1,  77) },
        { S(  13,  38), S(  -2,  85), S(  -0,  96), S(  -7, 113) },
        { S(   5,  46), S(  20,  65), S(  -1, 119), S(  -4, 124) },
        { S(  -5,  60), S( -48, 105), S(  -3, 114), S( -35, 154) },
        { S( -10,  73), S( -24,  97), S( -15, 115), S( -26, 111) }
    },

    // King
    {
        { S( 270,   2), S( 297,  49), S( 223,  79), S( 171,  97) },
        { S( 276,  50), S( 279,  83), S( 244, 107), S( 212, 115) },
        { S( 193,  83), S( 251,  96), S( 231, 119), S( 223, 129) },
        { S( 157,  92), S( 249, 113), S( 243, 129), S( 225, 145) },
        { S( 198, 116), S( 279, 141), S( 265, 152), S( 230, 163) },
        { S( 231, 140), S( 327, 172), S( 308, 169), S( 287, 159) },
        { S( 215,  96), S( 274, 181), S( 306, 163), S( 292, 151) },
        { S( 277,-157), S( 353,  43), S( 324,  71), S( 268,  97) }
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
