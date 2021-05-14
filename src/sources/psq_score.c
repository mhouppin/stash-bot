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
    { S( -7,  4), S(  3,  4), S( 10, 18), S(  5, 15), S(  0, 14), S( 13, 21), S( 10,  2), S( -2, -4) },
    { S( -9,  4), S(  1,  5), S(  4,  7), S(  7,  2), S( 13,  1), S(  4,  8), S(  5,  5), S( -5,  0) },
    { S(-11,  8), S( -8,  9), S(  6, -6), S( 14,-18), S( 15,-20), S( 10, -6), S(-10,  8), S(-10,  7) },
    { S(  7, 22), S(  7, 21), S( 25, -2), S( 46,-25), S( 45,-24), S( 31, -9), S(  6, 19), S(  2, 20) },
    { S(  5, 68), S( 58, 58), S( 70, 22), S( 73,-16), S( 77,-19), S( 84, 11), S( 60, 50), S(  6, 66) },
    { S( 33, 22), S( -1, 41), S( 62,-18), S( 69,-55), S( 83,-61), S( 59,-28), S(-36, 38), S(-26, 39) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -48, -69), S( -18, -32), S( -10, -16), S(  -8,  -2) },
        { S( -11, -10), S( -25,  -7), S(  -8, -15), S(   1,   3) },
        { S( -18, -34), S(  -4, -12), S(   1,  13), S(   7,  26) },
        { S(  12,  -6), S(  26,   9), S(  20,  35), S(  22,  44) },
        { S(   6,  -9), S(  25,  16), S(  36,  36), S(  31,  53) },
        { S( -13, -11), S(  32,  -4), S(  28,  45), S(  30,  40) },
        { S( -18, -31), S( -22,  -6), S(  34,  -6), S(  58,   7) },
        { S(-166, -86), S(-124,  -2), S(-126,  34), S( -37,   4) }
    },

    // Bishop
    {
        { S(  11, -31), S(   0,   1), S(   0,   1), S(  -8,   1) },
        { S(   8, -25), S(   9, -23), S(   1,  -5), S(  -4,   5) },
        { S(   5, -11), S(  10,   8), S(   7,  14), S(   9,  16) },
        { S(  -8,  -5), S(   2,   7), S(  15,  27), S(  22,  28) },
        { S( -10,  12), S(  16,  25), S(  16,  22), S(  41,  30) },
        { S(  -7,  23), S(  21,  31), S(  29,  25), S(  28,  16) },
        { S( -63,  19), S( -34,  32), S( -12,  27), S( -30,  17) },
        { S( -77,  40), S( -80,  33), S(-156,  40), S(-141,  47) }
    },

    // Rook
    {
        { S( -34,  -5), S( -16,   1), S(  -9,   4), S(  -5,  -3) },
        { S(-101,   2), S( -26, -10), S( -19,  -5), S( -19,  -4) },
        { S( -45,  -6), S( -17,   9), S( -31,  10), S( -28,   7) },
        { S( -41,  21), S( -24,  32), S( -31,  34), S( -30,  30) },
        { S( -12,  41), S(   8,  35), S(  11,  36), S(  30,  30) },
        { S(  -9,  53), S(  44,  26), S(  21,  44), S(  46,  30) },
        { S( -14,  62), S( -15,  68), S(   8,  63), S(   9,  67) },
        { S(  25,  63), S(   7,  72), S( -10,  78), S(  -9,  76) }
    },

    // Queen
    {
        { S(  21, -95), S(  29, -88), S(  27, -91), S(  27, -63) },
        { S(   9, -70), S(  26, -76), S(  31, -81), S(  25, -39) },
        { S(  14, -48), S(  22, -31), S(  15,   1), S(  15,  -9) },
        { S(  14,   0), S(  18,  14), S(   4,  38), S(   3,  63) },
        { S(  11,  38), S(  16,  66), S(   6,  66), S(   1,  95) },
        { S(  10,  53), S(  20,  36), S(  11,  87), S(   7,  92) },
        { S( -11,  34), S( -64,  95), S( -15, 101), S( -41, 140) },
        { S(   5,  39), S( -19,  80), S(  -2,  90), S(  -2, 106) }
    },

    // King
    {
        { S( 255, -16), S( 259,  47), S( 218,  70), S( 198,  51) },
        { S( 258,  64), S( 243,  91), S( 215, 114), S( 179, 126) },
        { S( 171,  94), S( 231, 108), S( 231, 131), S( 223, 148) },
        { S( 160,  95), S( 261, 127), S( 253, 154), S( 224, 172) },
        { S( 158, 124), S( 257, 160), S( 253, 175), S( 221, 182) },
        { S( 183, 134), S( 266, 181), S( 253, 183), S( 205, 175) },
        { S( 167,  73), S( 202, 179), S( 206, 162), S( 174, 146) },
        { S(  92, -92), S( 228,  65), S( 167,  91), S( 130, 117) }
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
