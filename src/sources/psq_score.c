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
    { S(-12, 25), S( -6, 21), S(-17, 21), S(  0, -3), S(  1, 15), S( 30, 16), S( 39,  1), S(  9,-15) },
    { S(-11, 16), S(-20, 23), S( -5, 13), S( -2,  4), S(  6, 13), S(  3, 15), S( 23,-12), S( 11,-12) },
    { S(-13, 28), S(-10, 17), S(  3, -7), S(  9,-20), S( 13,-14), S( 16, -6), S( 17,-10), S(  4, -7) },
    { S( -4, 45), S( -4, 25), S(  2,  8), S( 24,-22), S( 36,-16), S( 48,-19), S( 27, -2), S( 10, 13) },
    { S(  2, 67), S(  4, 48), S( 21, 23), S( 24,-16), S( 49,-27), S(125,-10), S( 72, 15), S( 36, 31) },
    { S( 88, 26), S( 86, 19), S( 90,  1), S(100,-53), S(127,-58), S( 76,-22), S(-77, 44), S(-44, 48) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -66, -51), S( -21, -34), S( -26, -15), S( -22,   9) },
        { S( -26, -10), S( -25,   7), S(  -9,  -9), S(  -7,   9) },
        { S( -22, -27), S(  -5,   5), S(  -5,  11), S(   3,  37) },
        { S(  -8,  16), S(  13,  29), S(   5,  48), S(   8,  52) },
        { S(   5,  29), S(   3,  34), S(  21,  49), S(   6,  67) },
        { S( -12,  19), S(  12,  32), S(  28,  54), S(  22,  53) },
        { S(  -6,  -6), S( -16,  20), S(  37,  14), S(  51,  24) },
        { S(-179, -49), S(-124,  31), S(-119,  59), S(   6,  19) }
    },

    // Bishop
    {
        { S(   2,  -0), S(  -1,   9), S( -19,   9), S( -21,  21) },
        { S(   4,  -4), S(   4,   0), S(  -0,   6), S( -12,  22) },
        { S(  -4,  17), S(   4,  22), S(  -2,  32), S(  -7,  44) },
        { S(  -3,   9), S(   1,  29), S(  -8,  50), S(  14,  55) },
        { S(  -8,  23), S(  -5,  46), S(  16,  39), S(  16,  60) },
        { S(   2,  26), S(  10,  49), S(  25,  39), S(  22,  35) },
        { S( -46,  37), S( -31,  44), S( -18,  46), S( -14,  40) },
        { S( -68,  60), S( -59,  50), S(-159,  66), S(-127,  62) }
    },

    // Rook
    {
        { S( -29,   4), S( -27,   8), S( -22,  13), S( -17,   6) },
        { S( -55,  10), S( -43,   7), S( -31,   6), S( -33,  13) },
        { S( -43,  20), S( -32,  23), S( -47,  29), S( -41,  28) },
        { S( -53,  47), S( -36,  49), S( -39,  51), S( -31,  44) },
        { S( -27,  61), S(  -6,  69), S(  -1,  61), S(  15,  53) },
        { S( -19,  71), S(  22,  58), S(  26,  67), S(  49,  47) },
        { S(  -1,  74), S(  -9,  79), S(  29,  66), S(  40,  63) },
        { S(  16,  66), S(  22,  64), S(   4,  70), S(  10,  63) }
    },

    // Queen
    {
        { S(  18, -54), S(  11, -76), S(  16, -83), S(  23, -57) },
        { S(  13, -55), S(  24, -57), S(  23, -46), S(  23, -40) },
        { S(  16, -30), S(  15,  -2), S(  10,  21), S(   4,  23) },
        { S(   7,  22), S(  11,  41), S(  -3,  62), S(   0,  80) },
        { S(  14,  37), S(  -7,  86), S(  -2,  95), S( -10, 116) },
        { S(   4,  47), S(  21,  62), S(   2, 120), S(  -4, 123) },
        { S(  -5,  57), S( -47, 102), S(  -2, 118), S( -39, 157) },
        { S(  -9,  74), S( -21, 101), S( -14, 118), S( -23, 115) }
    },

    // King
    {
        { S( 267,  -4), S( 293,  47), S( 220,  79), S( 167,  93) },
        { S( 274,  49), S( 272,  85), S( 240, 108), S( 208, 117) },
        { S( 194,  79), S( 249,  94), S( 232, 118), S( 223, 129) },
        { S( 157,  88), S( 251, 114), S( 248, 129), S( 229, 143) },
        { S( 200, 120), S( 283, 142), S( 269, 152), S( 234, 165) },
        { S( 231, 140), S( 334, 176), S( 320, 175), S( 291, 163) },
        { S( 212,  92), S( 272, 186), S( 307, 169), S( 290, 156) },
        { S( 275,-159), S( 348,  40), S( 319,  67), S( 265,  94) }
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
