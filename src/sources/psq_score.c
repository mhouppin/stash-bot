/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

// Square-based Pawn scoring for evaluation
const scorepair_t PawnBonus[RANK_NB][FILE_NB] = {
    { },
    { S( -9, 20), S( -5, 20), S(-13, 14), S(  0,  2), S(  0, 13), S( 35, 21), S( 35, 12), S(  6,-15) },
    { S( -8, 15), S(-17, 18), S(  1,  8), S(  3,  4), S(  8, 12), S(  6, 13), S( 20, -3), S(  7,-11) },
    { S(-10, 20), S( -9, 14), S(  6, -8), S( 12,-18), S( 20,-12), S( 18, -5), S( 14, -6), S( -4, -9) },
    { S( -2, 37), S( -6, 21), S(  3,  2), S( 22,-21), S( 36,-12), S( 43,-12), S( 17,  5), S(  9,  9) },
    { S(  4, 53), S(  4, 40), S( 25, 10), S( 40,-23), S( 48,-20), S(111,  3), S( 65, 23), S( 29, 26) },
    { S( 96, 24), S( 82, 24), S( 87,  1), S(105,-37), S(122,-40), S( 70,-16), S(-67, 31), S(-46, 35) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -64, -37), S( -13, -38), S( -29,  -9), S( -19,   8) },
        { S( -21, -11), S( -24,   7), S( -12,  -8), S(  -5,   7) },
        { S( -15, -19), S(  -6,   2), S(  -0,   7), S(   7,  30) },
        { S(  -7,  22), S(  13,  31), S(   7,  41), S(  11,  48) },
        { S(   3,  37), S(   3,  36), S(  25,  41), S(   8,  56) },
        { S( -32,  24), S(   3,  34), S(  22,  43), S(  31,  45) },
        { S( -11,   5), S( -24,  20), S(  36,  17), S(  40,  30) },
        { S(-174, -42), S(-122,  27), S(-117,  45), S(  12,  24) }
    },

    // Bishop
    {
        { S(  -4, -10), S(   8,   7), S( -13,  13), S( -21,  20) },
        { S(   1,   1), S(   4,   0), S(   5,  19), S( -10,  29) },
        { S(  -2,  16), S(   5,  29), S(  -3,  23), S(  -1,  51) },
        { S(  -2,   5), S(   4,  37), S(  -2,  55), S(  17,  65) },
        { S( -15,  24), S(   2,  50), S(  18,  52), S(  21,  69) },
        { S(   3,  28), S(   7,  51), S(  20,  34), S(  21,  47) },
        { S( -62,  30), S( -56,  27), S( -23,  45), S( -21,  41) },
        { S( -68,  36), S( -58,  44), S(-151,  51), S(-117,  48) }
    },

    // Rook
    {
        { S( -30,   3), S( -27,  12), S( -20,  15), S( -16,  12) },
        { S( -59,   5), S( -39,   7), S( -28,  11), S( -28,  12) },
        { S( -49,  19), S( -29,  23), S( -40,  29), S( -36,  26) },
        { S( -46,  39), S( -35,  46), S( -36,  46), S( -26,  36) },
        { S( -29,  57), S( -16,  64), S(   2,  56), S(  12,  56) },
        { S( -19,  66), S(  13,  61), S(  20,  65), S(  41,  55) },
        { S(  -4,  71), S( -10,  72), S(  31,  70), S(  40,  72) },
        { S(  11,  65), S(  21,  69), S(   7,  67), S(  15,  63) }
    },

    // Queen
    {
        { S(  15, -45), S(  11, -55), S(  16, -65), S(  20, -33) },
        { S(  12, -41), S(  19, -40), S(  22, -31), S(  19, -13) },
        { S(  14, -16), S(  16,   6), S(  11,  29), S(   3,  29) },
        { S(   6,  29), S(  16,  38), S(   0,  61), S(   2,  73) },
        { S(  14,  37), S(  -0,  78), S(   1,  83), S(  -5,  98) },
        { S(   6,  43), S(   6,  68), S(  -4, 107), S(  -5, 110) },
        { S( -12,  58), S( -47,  90), S(  -6, 104), S( -31, 133) },
        { S( -14,  70), S( -27,  85), S( -20, 102), S( -25, 107) }
    },

    // King
    {
        { S( 282,  12), S( 298,  57), S( 224,  69), S( 164,  81) },
        { S( 289,  57), S( 274,  86), S( 247,  99), S( 225, 103) },
        { S( 193,  72), S( 256,  91), S( 233, 110), S( 228, 118) },
        { S( 149,  77), S( 238, 109), S( 235, 124), S( 222, 133) },
        { S( 192, 105), S( 275, 145), S( 260, 152), S( 230, 154) },
        { S( 231, 135), S( 317, 176), S( 298, 180), S( 285, 165) },
        { S( 217, 104), S( 275, 177), S( 301, 171), S( 294, 157) },
        { S( 280,-147), S( 359,  62), S( 332,  92), S( 272, 113) }
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

            // Locate the square entry based on the piece type and square.
            if (piece == WHITE_PAWN)
                psqEntry = pieceValue + PawnBonus[sq_rank(square)][sq_file(square)];

            else
            {
                // Map squares for pieces on the queenside.
                file_t queensideFile = imin(sq_file(square), sq_file(square) ^ 7);

                psqEntry = pieceValue + PieceBonus[piece][sq_rank(square)][queensideFile];
            }

            // Assign the score twice, once for White and once for Black (with
            // the square mirrored horizontally).
            PsqScore[piece][square] = psqEntry;
            PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
        }
    }
}
