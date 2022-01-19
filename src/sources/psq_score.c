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
    { S(-14, 24), S( -6, 18), S(-18, 21), S( -1, -7), S( -1, 17), S( 29, 16), S( 40,  1), S( 11,-18) },
    { S(-14, 18), S(-21, 22), S( -6, 11), S( -2,  3), S(  7, 12), S(  3, 17), S( 23,-13), S( 11,-13) },
    { S(-14, 29), S(-10, 16), S(  2, -8), S( 11,-23), S( 12,-13), S( 16, -5), S( 19, -8), S(  5, -7) },
    { S( -7, 48), S( -6, 26), S(  1,  8), S( 24,-22), S( 36,-16), S( 47,-18), S( 31, -2), S(  9, 12) },
    { S(  2, 70), S(  4, 50), S( 20, 21), S( 28,-17), S( 49,-26), S(125,-11), S( 73, 17), S( 35, 31) },
    { S( 88, 27), S( 88, 19), S( 90,  1), S(100,-52), S(128,-57), S( 76,-23), S(-77, 45), S(-43, 48) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -66, -52), S( -22, -36), S( -30, -16), S( -21,   9) },
        { S( -25,  -9), S( -25,   6), S(  -8, -10), S(  -8,   9) },
        { S( -24, -27), S(  -9,   9), S(  -5,  12), S(   5,  38) },
        { S(  -8,  16), S(  16,  30), S(   6,  51), S(   7,  51) },
        { S(   6,  28), S(   4,  31), S(  21,  50), S(   8,  67) },
        { S( -10,  19), S(  14,  33), S(  28,  57), S(  25,  55) },
        { S(  -7,  -7), S( -18,  18), S(  36,  13), S(  53,  24) },
        { S(-180, -50), S(-124,  30), S(-120,  58), S(   5,  18) }
    },

    // Bishop
    {
        { S(   2,   2), S(  -0,  10), S( -19,   8), S( -22,  22) },
        { S(   5,  -4), S(   5,   2), S(   0,   5), S( -12,  20) },
        { S(  -5,  18), S(   4,  21), S(  -0,  32), S(  -6,  43) },
        { S(  -2,  10), S(   4,  29), S(  -9,  51), S(  14,  57) },
        { S(  -9,  23), S(  -6,  45), S(  16,  39), S(  19,  60) },
        { S(  -1,  25), S(  11,  50), S(  25,  38), S(  21,  32) },
        { S( -47,  35), S( -32,  44), S( -18,  46), S( -14,  41) },
        { S( -69,  60), S( -60,  49), S(-159,  67), S(-128,  62) }
    },

    // Rook
    {
        { S( -30,   2), S( -27,  10), S( -21,  14), S( -17,   5) },
        { S( -56,   9), S( -42,   7), S( -30,   8), S( -33,  10) },
        { S( -47,  19), S( -35,  22), S( -51,  29), S( -42,  27) },
        { S( -52,  46), S( -34,  50), S( -40,  51), S( -34,  46) },
        { S( -25,  63), S(  -7,  68), S(  -0,  61), S(  13,  52) },
        { S( -18,  72), S(  24,  58), S(  26,  64), S(  48,  47) },
        { S(  -1,  74), S( -10,  75), S(  31,  68), S(  42,  63) },
        { S(  16,  68), S(  22,  65), S(   4,  71), S(  10,  64) }
    },

    // Queen
    {
        { S(  18, -54), S(  13, -77), S(  16, -83), S(  21, -60) },
        { S(  11, -57), S(  21, -58), S(  23, -49), S(  23, -38) },
        { S(  18, -30), S(  15,  -3), S(   9,  21), S(   3,  23) },
        { S(   6,  21), S(   8,  40), S(  -3,  62), S(  -1,  82) },
        { S(  12,  36), S(  -5,  86), S(  -1,  95), S( -11, 116) },
        { S(   6,  48), S(  24,  62), S(   4, 121), S(  -4, 123) },
        { S(  -3,  57), S( -47, 101), S(  -2, 118), S( -37, 159) },
        { S(  -9,  75), S( -21, 102), S( -13, 120), S( -21, 117) }
    },

    // King
    {
        { S( 267,  -5), S( 293,  46), S( 220,  78), S( 166,  89) },
        { S( 272,  48), S( 270,  85), S( 238, 109), S( 206, 117) },
        { S( 194,  74), S( 249,  93), S( 232, 119), S( 222, 129) },
        { S( 157,  88), S( 251, 112), S( 249, 129), S( 230, 144) },
        { S( 200, 121), S( 284, 142), S( 271, 154), S( 235, 165) },
        { S( 231, 140), S( 335, 177), S( 324, 178), S( 292, 165) },
        { S( 211,  91), S( 272, 189), S( 308, 173), S( 290, 158) },
        { S( 275,-159), S( 347,  39), S( 318,  66), S( 264,  93) }
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
