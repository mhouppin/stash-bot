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
    { S( -9, 17), S( -3, 21), S(-16, 13), S(  2,  5), S(  2, 11), S( 34, 21), S( 41, 12), S( 10,-16) },
    { S( -9, 16), S(-18, 18), S( -3,  8), S(  0,  5), S(  7, 11), S(  7, 15), S( 24, -5), S(  9,-12) },
    { S(-11, 19), S(-12, 13), S(  2, -7), S(  6,-20), S( 15,-15), S( 14, -5), S( 11, -7), S(  2,-10) },
    { S( -4, 39), S( -5, 23), S(  6,  2), S( 22,-20), S( 36,-10), S( 46,-11), S( 16,  4), S( 10, 10) },
    { S(  1, 53), S( 10, 41), S( 25,  9), S( 29,-18), S( 53,-19), S(111,  3), S( 70, 20), S( 28, 29) },
    { S( 95, 27), S( 82, 23), S( 89,  1), S(104,-40), S(124,-48), S( 72,-19), S(-72, 34), S(-46, 44) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -66, -42), S( -19, -35), S( -30, -13), S( -17,   9) },
        { S( -23, -13), S( -22,   8), S( -12,  -8), S(  -6,   4) },
        { S( -20, -20), S(  -8,   1), S(  -5,   8), S(   8,  31) },
        { S( -10,  24), S(  21,  29), S(   7,  42), S(   8,  50) },
        { S(   2,  38), S(   2,  35), S(  25,  46), S(  10,  59) },
        { S( -26,  19), S(   7,  35), S(  22,  43), S(  31,  47) },
        { S( -14,   4), S( -21,  18), S(  33,  15), S(  41,  31) },
        { S(-174, -43), S(-122,  32), S(-117,  53), S(  10,  23) }
    },

    // Bishop
    {
        { S(   2,  -5), S(   2,   5), S( -14,   6), S( -23,  18) },
        { S(  -7,  -2), S(   6,   8), S(   4,  16), S( -11,  23) },
        { S(  -4,  16), S(   1,  28), S(  -1,  38), S(  -2,  44) },
        { S(  -5,   6), S(   3,  31), S(  -6,  49), S(  15,  59) },
        { S( -16,  21), S(  -2,  47), S(  17,  45), S(  19,  63) },
        { S(   3,  27), S(   6,  48), S(  24,  48), S(  19,  40) },
        { S( -55,  31), S( -44,  40), S( -22,  45), S( -21,  42) },
        { S( -63,  55), S( -58,  47), S(-151,  58), S(-119,  52) }
    },

    // Rook
    {
        { S( -30,   2), S( -28,  11), S( -21,  14), S( -18,  11) },
        { S( -59,   4), S( -40,   4), S( -30,  11), S( -30,  12) },
        { S( -46,  21), S( -33,  21), S( -42,  25), S( -39,  25) },
        { S( -49,  40), S( -36,  48), S( -39,  44), S( -27,  36) },
        { S( -28,  59), S( -13,  64), S(  -3,  59), S(  14,  57) },
        { S( -18,  68), S(  18,  62), S(  21,  66), S(  43,  54) },
        { S(  -7,  71), S( -11,  75), S(  30,  74), S(  37,  71) },
        { S(  12,  67), S(  21,  69), S(   7,  68), S(  15,  62) }
    },

    // Queen
    {
        { S(  24, -48), S(  16, -63), S(  13, -74), S(  19, -36) },
        { S(  10, -47), S(  22, -46), S(  22, -39), S(  22, -20) },
        { S(  20, -25), S(  16,   0), S(   9,  25), S(   4,  24) },
        { S(   4,  28), S(  15,  42), S(  -1,  61), S(  -1,  75) },
        { S(   9,  39), S(  -2,  79), S(  -2,  90), S(  -6, 101) },
        { S(   2,  40), S(  14,  66), S(  -0, 111), S(  -7, 118) },
        { S( -12,  57), S( -49,  96), S(  -1, 112), S( -35, 142) },
        { S( -14,  70), S( -27,  89), S( -18, 109), S( -24, 114) }
    },

    // King
    {
        { S( 283,  10), S( 296,  58), S( 224,  69), S( 167,  81) },
        { S( 290,  56), S( 273,  87), S( 247,  99), S( 219, 104) },
        { S( 190,  71), S( 256,  91), S( 231, 111), S( 225, 120) },
        { S( 152,  81), S( 241, 110), S( 237, 126), S( 221, 134) },
        { S( 194, 109), S( 277, 149), S( 260, 155), S( 228, 155) },
        { S( 231, 140), S( 319, 179), S( 298, 182), S( 285, 171) },
        { S( 217, 102), S( 275, 177), S( 303, 170), S( 294, 153) },
        { S( 280,-152), S( 358,  52), S( 332,  83), S( 272, 106) }
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
