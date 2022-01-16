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
    { S(-16, 23), S( -6, 20), S(-14, 20), S( -3, -7), S(  1, 16), S( 29, 14), S( 40,  1), S(  9,-16) },
    { S(-13, 15), S(-20, 21), S( -2, 10), S( -2,  1), S(  8, 11), S(  1, 17), S( 25,-10), S( 10,-13) },
    { S(-16, 29), S(-12, 16), S(  2, -6), S( 10,-25), S( 11,-16), S( 16, -2), S( 19, -8), S(  6,-10) },
    { S(-10, 49), S( -7, 26), S(  1,  8), S( 21,-24), S( 32,-17), S( 47,-18), S( 32,  1), S( 10, 13) },
    { S(  4, 70), S(  2, 50), S( 21, 21), S( 29,-16), S( 50,-25), S(125,-10), S( 73, 17), S( 36, 32) },
    { S( 88, 28), S( 89, 19), S( 90,  1), S(100,-51), S(129,-56), S( 76,-23), S(-76, 45), S(-43, 48) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -65, -52), S( -25, -37), S( -29, -16), S( -22,   8) },
        { S( -27,  -9), S( -25,   6), S( -14, -10), S(  -8,   9) },
        { S( -26, -27), S(  -9,  10), S(  -5,  14), S(   9,  38) },
        { S(  -7,  17), S(  18,  31), S(   7,  49), S(   7,  52) },
        { S(   5,  27), S(   1,  30), S(  20,  49), S(   9,  66) },
        { S(  -9,  19), S(  17,  33), S(  27,  59), S(  29,  57) },
        { S(  -7,  -8), S( -18,  18), S(  36,  13), S(  53,  24) },
        { S(-180, -51), S(-124,  30), S(-120,  58), S(   5,  18) }
    },

    // Bishop
    {
        { S(  -0,   2), S(   1,  11), S( -18,   8), S( -23,  22) },
        { S(   5,  -5), S(   6,   1), S(  -1,   6), S( -11,  20) },
        { S(  -4,  20), S(   4,  21), S(   2,  30), S(  -7,  43) },
        { S(  -3,  10), S(   6,  29), S(  -8,  53), S(  14,  57) },
        { S( -10,  22), S(  -7,  44), S(  17,  40), S(  19,  60) },
        { S(  -2,  25), S(  11,  51), S(  26,  38), S(  19,  30) },
        { S( -47,  34), S( -33,  44), S( -18,  46), S( -13,  42) },
        { S( -69,  60), S( -60,  48), S(-159,  68), S(-129,  62) }
    },

    // Rook
    {
        { S( -31,   1), S( -26,  10), S( -21,  11), S( -19,   4) },
        { S( -57,   8), S( -40,   7), S( -29,   9), S( -34,   8) },
        { S( -52,  18), S( -36,  22), S( -49,  30), S( -43,  27) },
        { S( -49,  46), S( -33,  52), S( -40,  50), S( -36,  46) },
        { S( -27,  62), S(  -7,  67), S(   0,  61), S(  13,  53) },
        { S( -17,  73), S(  25,  60), S(  27,  65), S(  47,  45) },
        { S(  -0,  75), S( -11,  73), S(  32,  69), S(  43,  64) },
        { S(  16,  70), S(  23,  66), S(   4,  72), S(   9,  65) }
    },

    // Queen
    {
        { S(  17, -54), S(  12, -78), S(  16, -83), S(  19, -61) },
        { S(  10, -58), S(  20, -59), S(  21, -50), S(  21, -37) },
        { S(  20, -30), S(  14,  -4), S(  14,  23), S(   1,  23) },
        { S(   6,  20), S(   9,  40), S(  -6,  61), S(  -3,  82) },
        { S(  14,  36), S(  -4,  86), S(   0,  95), S( -11, 116) },
        { S(   8,  49), S(  26,  62), S(   4, 121), S(  -4, 123) },
        { S(  -2,  57), S( -47, 101), S(  -2, 118), S( -37, 160) },
        { S(  -9,  75), S( -20, 103), S( -13, 120), S( -20, 118) }
    },

    // King
    {
        { S( 267,  -6), S( 292,  44), S( 222,  78), S( 165,  88) },
        { S( 269,  46), S( 273,  84), S( 233, 109), S( 204, 116) },
        { S( 193,  71), S( 249,  93), S( 232, 120), S( 223, 128) },
        { S( 158,  89), S( 251, 111), S( 250, 131), S( 230, 143) },
        { S( 200, 122), S( 285, 143), S( 272, 155), S( 236, 166) },
        { S( 231, 140), S( 336, 178), S( 326, 180), S( 293, 166) },
        { S( 211,  91), S( 272, 190), S( 308, 175), S( 290, 159) },
        { S( 275,-159), S( 346,  38), S( 317,  66), S( 264,  93) }
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
