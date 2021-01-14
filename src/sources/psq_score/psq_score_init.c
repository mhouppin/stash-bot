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

scorepair_t     PsqScore[PIECE_NB][SQUARE_NB];
const score_t   PieceScores[PHASE_NB][PIECE_NB] = {
    {
        0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE,
        ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0,
        0, -PAWN_MG_SCORE, -KNIGHT_MG_SCORE, -BISHOP_MG_SCORE,
        -ROOK_MG_SCORE, -QUEEN_MG_SCORE, 0, 0
    },
    {
        0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE,
        ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0,
        0, -PAWN_EG_SCORE, -KNIGHT_EG_SCORE, -BISHOP_EG_SCORE,
        -ROOK_EG_SCORE, -QUEEN_EG_SCORE, 0, 0,
    }
};

#define S SPAIR

const scorepair_t   PawnBonus[RANK_NB][FILE_NB] = {
    { },
    { S(-14, 19), S(-14,  7), S(-11, 12), S( -6,  9), S( -7, 19), S( 35, 15), S( 33, -1), S(  7,-18) },
    { S(-11, 12), S(-11,  7), S( -2,  2), S(  3,  1), S( 18,  8), S( 12,  7), S( 29, -7), S(  9, -7) },
    { S( -9, 20), S(-10, 14), S(  1, -6), S(  7,-14), S( 16,-11), S( 21, -5), S( 12, -3), S(  3, -4) },
    { S( -9, 40), S( -5, 28), S(  0, 11), S( 21,-18), S( 35,-16), S( 50,-11), S( 19,  6), S(  2,  9) },
    { S( -5, 81), S( 19, 61), S( 38, 25), S( 40,-19), S( 60,-29), S(123, -8), S( 55, 17), S( 25, 29) },
    { S( 68, 28), S( 58, 19), S( 56, -7), S( 69,-50), S( 53,-54), S( 32,-39), S(-48, -9), S(-34,  2) },
    { }
};

const scorepair_t   PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight

    {
        { S( -74, -75), S( -14, -61), S( -16, -36), S( -16, -25) },
        { S(  -5, -24), S( -22, -17), S( -10, -38), S(  -3, -20) },
        { S( -13, -60), S(   1, -25), S(  -8, -10), S(   6,  13) },
        { S(   8, -16), S(  21,  -3), S(  12,  29), S(  15,  32) },
        { S(  27, -19), S(  20,   5), S(  39,  28), S(  30,  38) },
        { S(   7, -32), S(  22,  -8), S(  45,  25), S(  49,  14) },
        { S(   6, -43), S( -12, -16), S(  46, -31), S(  39,   8) },
        { S(-104,-105), S( -13, -46), S( -44,  -2), S(  -5, -12) }
    },

    // Bishop

    {
        { S(  19, -16), S(  11,  -8), S( -13, -14), S( -26, -12) },
        { S(  17, -38), S(  12, -14), S(   9, -25), S(  -7, -12) },
        { S(   5,  -8), S(   5, -14), S(   2,  -3), S(   4,   1) },
        { S(   2, -18), S(   2,  -8), S(  -1,   3), S(  21,   5) },
        { S( -18,   3), S(  15,   5), S(  14,  -1), S(  31,   9) },
        { S(  -7,   5), S(   8,  10), S(  37,   5), S(  22,  -2) },
        { S( -48, -12), S( -14,   8), S( -11,   4), S( -21,   9) },
        { S( -41,  15), S( -14,   7), S( -74,  12), S( -72,  19) }
    },

    // Rook

    {
        { S( -26, -19), S( -16, -12), S( -13,  -8), S(  -8, -19) },
        { S( -65, -11), S( -23, -28), S( -23, -25), S( -28, -24) },
        { S( -38, -16), S( -20,  -8), S( -41,  -5), S( -33,  -9) },
        { S( -33,   8), S( -32,  21), S( -37,  22), S( -28,  14) },
        { S( -13,  23), S(   4,  21), S(   7,  23), S(  22,  17) },
        { S( -15,  34), S(  31,  19), S(  20,  33), S(  41,  15) },
        { S(   3,  35), S( -13,  43), S(  16,  36), S(  23,  43) },
        { S(  24,  51), S(  20,  54), S(  -8,  61), S(  -2,  53) }
    },

    // Queen

    {
        { S(  16, -85), S(  13, -86), S(  16,-102), S(  21, -71) },
        { S(  14, -73), S(  21, -86), S(  22, -96), S(  13, -59) },
        { S(   9, -52), S(  14, -39), S(   5, -11), S(   2, -23) },
        { S(   9, -12), S(  11,   1), S(   1,  18), S(  -9,  51) },
        { S(  27,  -1), S(  12,  43), S(  -1,  51), S( -19,  86) },
        { S(  20,  14), S(  28,  12), S(   3,  65), S(   2,  64) },
        { S(   2,  18), S(  36,  15), S(   5,  48), S( -13,  85) },
        { S(  38,  -1), S(  45,  18), S(  52,  37), S(  47,  40) }
    },

    // King

    {
        { S( 284, -63), S( 283,  35), S( 219,  60), S( 150,  55) },
        { S( 284,  40), S( 263,  87), S( 205, 124), S( 185, 129) },
        { S( 186,  86), S( 262, 106), S( 197, 143), S( 171, 166) },
        { S( 152,  95), S( 224, 146), S( 192, 177), S( 157, 198) },
        { S( 153, 124), S( 197, 181), S( 134, 205), S(  97, 208) },
        { S( 131, 126), S( 163, 208), S( 104, 215), S(  45, 196) },
        { S(  93,  56), S( 131, 178), S(  80, 163), S(  44, 152) },
        { S(  59,  -3), S(  92,  63), S(  48,  78), S(   3,  84) }
    }
};

#undef S

void    psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t piece_value = create_scorepair(
            PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

        for (square_t square = SQ_A1; square <= SQ_H8; ++square)
        {
            scorepair_t psq_entry;

            if (piece == WHITE_PAWN)
                psq_entry = piece_value + PawnBonus[rank_of_square(square)][file_of_square(square)];
            else
            {
                file_t  qside_file = min(file_of_square(square), file_of_square(square) ^ 7);

                psq_entry = piece_value + PieceBonus[piece][rank_of_square(square)][qside_file];
            }

            PsqScore[piece][square] = psq_entry;
            PsqScore[opposite_piece(piece)][opposite_square(square)] = -psq_entry;
        }
    }
}
