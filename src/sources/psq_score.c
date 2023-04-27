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
    { S( -9, 21), S( -7, 20), S(-18, 13), S( -2,  2), S( -2, 13), S( 35, 21), S( 37, 13), S(  6,-15) },
    { S( -7, 15), S(-19, 17), S(  1,  8), S(  2,  5), S(  8, 10), S(  5, 14), S( 20, -4), S(  7,-12) },
    { S(-11, 21), S( -9, 13), S(  6, -8), S( 12,-19), S( 20,-14), S( 17, -5), S( 14, -5), S( -4, -9) },
    { S(  0, 39), S( -4, 22), S(  3,  2), S( 24,-20), S( 40,-13), S( 44,-11), S( 16,  5), S(  9,  9) },
    { S(  5, 54), S(  5, 40), S( 26, 11), S( 41,-22), S( 49,-20), S(112,  4), S( 65, 22), S( 29, 26) },
    { S( 97, 25), S( 82, 24), S( 87,  1), S(105,-36), S(122,-39), S( 70,-16), S(-67, 31), S(-46, 35) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -64, -37), S( -16, -38), S( -30, -10), S( -19,   7) },
        { S( -22, -11), S( -24,   7), S( -12,  -8), S(  -6,   7) },
        { S( -16, -19), S(  -6,   2), S(  -1,   7), S(  10,  31) },
        { S(  -8,  22), S(  14,  31), S(   9,  41), S(  12,  48) },
        { S(   3,  37), S(   3,  37), S(  27,  41), S(   9,  57) },
        { S( -32,  24), S(   3,  34), S(  22,  43), S(  32,  45) },
        { S( -11,   5), S( -24,  20), S(  36,  17), S(  40,  30) },
        { S(-174, -42), S(-122,  27), S(-117,  45), S(  12,  24) }
    },

    // Bishop
    {
        { S(  -5, -11), S(   8,   7), S( -15,  13), S( -22,  19) },
        { S(   1,   1), S(   4,  -1), S(   6,  19), S( -11,  28) },
        { S(  -2,  16), S(   5,  29), S(  -3,  22), S(   0,  52) },
        { S(  -2,   5), S(   4,  37), S(  -1,  56), S(  19,  66) },
        { S( -15,  24), S(   3,  51), S(  19,  52), S(  22,  70) },
        { S(   3,  28), S(   7,  52), S(  20,  34), S(  21,  47) },
        { S( -63,  30), S( -57,  27), S( -23,  45), S( -21,  41) },
        { S( -68,  35), S( -58,  44), S(-151,  51), S(-117,  48) }
    },

    // Rook
    {
        { S( -31,   2), S( -28,  10), S( -19,  14), S( -15,  11) },
        { S( -59,   5), S( -39,   6), S( -29,  10), S( -29,  10) },
        { S( -50,  19), S( -30,  23), S( -41,  28), S( -37,  26) },
        { S( -47,  40), S( -34,  47), S( -36,  46), S( -26,  36) },
        { S( -28,  58), S( -16,  65), S(   3,  56), S(  13,  57) },
        { S( -18,  67), S(  13,  62), S(  21,  66), S(  41,  55) },
        { S(  -3,  72), S( -10,  73), S(  32,  71), S(  41,  73) },
        { S(  11,  65), S(  21,  69), S(   7,  67), S(  15,  63) }
    },

    // Queen
    {
        { S(  14, -45), S(  11, -55), S(  15, -65), S(  19, -34) },
        { S(  11, -41), S(  18, -40), S(  22, -32), S(  19, -14) },
        { S(  14, -16), S(  16,   6), S(  10,  29), S(   2,  29) },
        { S(   6,  29), S(  16,  38), S(   1,  61), S(   2,  74) },
        { S(  14,  37), S(   0,  78), S(   2,  83), S(  -4,  98) },
        { S(   7,  43), S(   5,  68), S(  -3, 108), S(  -5, 110) },
        { S( -12,  58), S( -47,  90), S(  -6, 104), S( -30, 133) },
        { S( -14,  70), S( -27,  85), S( -20, 102), S( -25, 107) }
    },

    // King
    {
        { S( 281,  11), S( 300,  57), S( 222,  67), S( 163,  80) },
        { S( 290,  57), S( 275,  87), S( 247,  99), S( 225, 103) },
        { S( 193,  71), S( 256,  91), S( 233, 110), S( 228, 118) },
        { S( 149,  77), S( 238, 109), S( 235, 125), S( 222, 133) },
        { S( 192, 105), S( 275, 145), S( 260, 152), S( 230, 154) },
        { S( 231, 135), S( 317, 177), S( 298, 180), S( 285, 166) },
        { S( 217, 104), S( 275, 177), S( 301, 171), S( 294, 157) },
        { S( 280,-147), S( 359,  63), S( 332,  93), S( 272, 113) }
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
