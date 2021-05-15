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
    { S(-13, 21), S(-15, 14), S(-16, 18), S( -4,  8), S(-10, 25), S( 35, 23), S( 36,  0), S( 12,-24) },
    { S(-11, 13), S(-16,  9), S( -6,  3), S( -1, -4), S(  9, 10), S( 18,  9), S( 24, -5), S( 13,-12) },
    { S(-14, 20), S(-12, 16), S( -3, -9), S(  1,-20), S( 11,-14), S( 18, -5), S( 17, -6), S(  5, -6) },
    { S( -9, 44), S(  0, 27), S(  0,  9), S( 21,-25), S( 43,-18), S( 53,-18), S( 32,  5), S(  9,  7) },
    { S( -8, 95), S( 16, 79), S( 34, 43), S( 37, -6), S( 51,-20), S(133, 11), S( 60, 32), S( 19, 42) },
    { S( 77, 37), S( 63, 32), S( 66,  2), S( 82,-51), S(101,-64), S( 55,-49), S( 59, 22), S( 67, 25) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -69, -72), S( -16, -37), S( -15, -16), S( -11,   6) },
        { S( -14, -10), S( -24,  -5), S(  -9, -14), S(  -5,   8) },
        { S( -18, -36), S(  -1,   0), S(  -5,  12), S(  10,  36) },
        { S(   0,  14), S(  30,  22), S(  10,  50), S(  11,  57) },
        { S(  27,  11), S(  18,  29), S(  33,  50), S(  18,  66) },
        { S(   6,   0), S(  11,  24), S(   9,  63), S(  14,  57) },
        { S(  -2, -27), S(  -9,   1), S(  35,  -7), S(  53,  26) },
        { S(-193, -53), S(-124,  21), S(-131,  49), S(  -5,  16) }
    },

    // Bishop
    {
        { S(  15,  -7), S(  10,   4), S( -10,   8), S( -24,  16) },
        { S(   4, -22), S(   7,  -6), S(   6,   1), S( -12,  13) },
        { S(  -1,   6), S(   8,  12), S(   2,  20), S(   2,  29) },
        { S(  -2,   6), S(   2,  18), S(   1,  38), S(  21,  37) },
        { S( -14,  28), S(  15,  35), S(  19,  29), S(  35,  42) },
        { S(  -9,  33), S(  16,  38), S(  32,  38), S(  24,  25) },
        { S( -39,  31), S( -23,  41), S( -15,  35), S( -30,  36) },
        { S( -72,  56), S( -61,  48), S(-161,  63), S(-135,  62) }
    },

    // Rook
    {
        { S( -31,   1), S( -22,   7), S( -17,  11), S( -12,   1) },
        { S( -72,   4), S( -30,  -7), S( -27,   0), S( -27,   0) },
        { S( -40,   3), S( -24,  21), S( -41,  21), S( -33,  18) },
        { S( -39,  31), S( -31,  44), S( -41,  48), S( -33,  41) },
        { S( -17,  51), S(  -1,  48), S(   8,  50), S(  24,  45) },
        { S( -14,  62), S(  33,  40), S(  17,  60), S(  41,  40) },
        { S(  -3,  68), S( -19,  76), S(  13,  65), S(  15,  77) },
        { S(  28,  75), S(  30,  77), S(  -6,  91), S(  -3,  85) }
    },

    // Queen
    {
        { S(  16, -68), S(  26, -94), S(  18, -79), S(  22, -51) },
        { S(  20, -74), S(  25, -74), S(  27, -74), S(  21, -40) },
        { S(  20, -51), S(  24, -21), S(  15,   6), S(  11,  -1) },
        { S(  14,   4), S(  18,  27), S(   4,  45), S(  -6,  76) },
        { S(  28,  20), S(  11,  80), S(   3,  89), S( -10, 115) },
        { S(  20,  44), S(  30,  39), S(   2, 111), S(  -2, 115) },
        { S(  -3,  52), S( -51,  93), S(  -9, 105), S( -50, 170) },
        { S(  -3,  72), S(  -7, 105), S(  -1, 121), S( -11, 128) }
    },

    // King
    {
        { S( 264, -27), S( 258,  53), S( 198,  77), S( 159,  58) },
        { S( 259,  57), S( 245,  93), S( 195, 125), S( 177, 129) },
        { S( 207,  85), S( 255, 103), S( 234, 135), S( 241, 151) },
        { S( 172,  98), S( 287, 126), S( 277, 156), S( 257, 172) },
        { S( 203, 119), S( 296, 155), S( 288, 173), S( 257, 179) },
        { S( 217, 126), S( 318, 175), S( 319, 179), S( 274, 162) },
        { S( 193,  78), S( 250, 175), S( 267, 150), S( 238, 139) },
        { S( 182,-115), S( 291,  51), S( 240,  72), S( 197,  99) }
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
