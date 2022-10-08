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

#include "psq_score.h"
#include "types.h"

// clang-format off

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
    { S( -9, 20), S( -5, 20), S(-14, 14), S(  0,  3), S(  1, 13), S( 35, 21), S( 36, 12), S(  6,-15) },
    { S( -8, 15), S(-17, 18), S(  1,  8), S(  3,  4), S(  7, 12), S(  7, 13), S( 20, -3), S(  6,-11) },
    { S(-11, 19), S(-10, 14), S(  6, -9), S( 11,-19), S( 19,-13), S( 17, -5), S( 15, -6), S( -4, -9) },
    { S( -2, 38), S( -6, 21), S(  2,  2), S( 22,-21), S( 35,-12), S( 43,-12), S( 18,  5), S(  9,  9) },
    { S(  3, 53), S(  5, 40), S( 25, 10), S( 38,-21), S( 49,-20), S(111,  3), S( 67, 22), S( 29, 27) },
    { S( 95, 25), S( 82, 24), S( 88,  1), S(105,-38), S(123,-43), S( 71,-17), S(-69, 32), S(-46, 38) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -65, -39), S( -14, -37), S( -29, -10), S( -20,   8) },
        { S( -21, -12), S( -24,   7), S( -12,  -8), S(  -5,   7) },
        { S( -16, -19), S(  -7,   2), S(  -0,   8), S(   7,  30) },
        { S(  -7,  23), S(  14,  30), S(   7,  42), S(  11,  49) },
        { S(   3,  37), S(   3,  36), S(  25,  42), S(   7,  57) },
        { S( -30,  22), S(   4,  34), S(  22,  44), S(  31,  46) },
        { S( -12,   5), S( -23,  19), S(  35,  17), S(  40,  30) },
        { S(-174, -42), S(-122,  29), S(-117,  48), S(  11,  23) }
    },

    // Bishop
    {
        { S(  -5,  -9), S(   7,   7), S( -13,  12), S( -22,  20) },
        { S(   0,   1), S(   3,   0), S(   4,  19), S( -11,  28) },
        { S(  -2,  17), S(   5,  29), S(  -4,  23), S(  -1,  49) },
        { S(  -2,   6), S(   3,  35), S(  -2,  54), S(  17,  63) },
        { S( -15,  24), S(   2,  49), S(  18,  50), S(  20,  68) },
        { S(   3,  28), S(   7,  50), S(  20,  37), S(  20,  45) },
        { S( -60,  30), S( -52,  31), S( -23,  45), S( -21,  41) },
        { S( -66,  44), S( -58,  45), S(-151,  53), S(-118,  49) }
    },

    // Rook
    {
        { S( -30,   3), S( -28,  12), S( -20,  15), S( -17,  13) },
        { S( -59,   4), S( -39,   7), S( -29,  11), S( -28,  12) },
        { S( -48,  20), S( -30,  23), S( -41,  29), S( -37,  26) },
        { S( -47,  39), S( -35,  46), S( -36,  46), S( -26,  36) },
        { S( -29,  58), S( -16,  63), S(   2,  57), S(  13,  56) },
        { S( -19,  66), S(  14,  61), S(  20,  65), S(  42,  55) },
        { S(  -5,  71), S( -11,  72), S(  31,  71), S(  39,  72) },
        { S(  11,  65), S(  21,  69), S(   7,  67), S(  15,  63) }
    },

    // Queen
    {
        { S(  17, -47), S(  12, -59), S(  17, -68), S(  20, -35) },
        { S(  12, -43), S(  20, -42), S(  22, -33), S(  19, -15) },
        { S(  15, -20), S(  16,   4), S(  11,  28), S(   3,  28) },
        { S(   6,  28), S(  16,  39), S(   0,  61), S(   2,  74) },
        { S(  13,  38), S(  -0,  79), S(   0,  86), S(  -5,  99) },
        { S(   6,  43), S(   8,  67), S(  -3, 108), S(  -6, 113) },
        { S( -12,  58), S( -48,  92), S(  -5, 106), S( -33, 136) },
        { S( -14,  70), S( -27,  86), S( -20, 104), S( -25, 109) }
    },

    // King
    {
        { S( 282,  11), S( 298,  57), S( 223,  69), S( 165,  82) },
        { S( 289,  57), S( 273,  87), S( 247,  99), S( 224, 103) },
        { S( 192,  72), S( 256,  91), S( 232, 110), S( 227, 119) },
        { S( 150,  77), S( 239, 110), S( 236, 124), S( 222, 134) },
        { S( 193, 106), S( 276, 146), S( 260, 153), S( 229, 155) },
        { S( 231, 137), S( 318, 177), S( 298, 181), S( 285, 167) },
        { S( 217, 103), S( 275, 177), S( 302, 171), S( 294, 156) },
        { S( 280,-149), S( 359,  58), S( 332,  88), S( 272, 110) }
    }
};

#undef S

// clang-format on

void psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t pieceValue =
            create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

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
