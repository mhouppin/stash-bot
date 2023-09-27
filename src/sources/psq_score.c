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
    { S(-32, 16), S(-27, 16), S(-36, 10), S(-21, -3), S(-22,  8), S( 14, 16), S( 13,  7), S(-17,-22) },
    { S(-30,  9), S(-40, 14), S(-20,  2), S(-19, -1), S(-13,  7), S(-14,  9), S( -1,-10), S(-15,-18) },
    { S(-33, 15), S(-31,  9), S(-15,-15), S( -9,-24), S( -1,-18), S( -2,-11), S( -7,-12), S(-25,-14) },
    { S(-22, 33), S(-28, 17), S(-19, -2), S(  1,-27), S( 14,-18), S( 23,-18), S( -4, -1), S(-12,  2) },
    { S(-16, 51), S(-17, 36), S(  5,  2), S( 22,-29), S( 28,-25), S( 92, -5), S( 43, 18), S(  8, 21) },
    { S( 80, 18), S( 63, 18), S( 64, -4), S( 86,-41), S( 97,-43), S( 44,-20), S(-79, 20), S(-67, 23) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -48), S(   2, -56), S( -14, -25), S(  -5,  -8) },
        { S(  -6, -27), S( -12,  -8), S(   3, -26), S(  10, -12) },
        { S(  -1, -37), S(   8, -15), S(  15, -12), S(  22,  13) },
        { S(  10,   6), S(  30,  14), S(  22,  25), S(  27,  32) },
        { S(  20,  23), S(  19,  20), S(  41,  25), S(  23,  42) },
        { S( -27,  11), S(  18,  20), S(  39,  25), S(  48,  26) },
        { S(   3, -11), S( -13,   6), S(  54,   1), S(  54,  17) },
        { S(-162, -58), S(-108,   7), S(-102,  23), S(  28,   7) }
    },

    // Bishop
    {
        { S(  15, -50), S(  24, -26), S(   3, -21), S(  -5, -12) },
        { S(  18, -33), S(  21, -35), S(  21, -14), S(   7,  -2) },
        { S(  14, -14), S(  22,  -3), S(  13, -13), S(  16,  21) },
        { S(  13, -30), S(  20,   7), S(  15,  25), S(  32,  37) },
        { S(   1,  -7), S(  17,  20), S(  33,  24), S(  35,  42) },
        { S(  21,  -3), S(  23,  21), S(  38,  -2), S(  39,  18) },
        { S( -54,   0), S( -47,  -8), S(  -8,  18), S(  -4,  12) },
        { S( -57, -13), S( -43,  11), S(-136,  17), S(-100,  15) }
    },

    // Rook
    {
        { S( -17, -42), S( -15, -31), S(  -8, -28), S(  -3, -33) },
        { S( -45, -38), S( -26, -38), S( -16, -33), S( -14, -33) },
        { S( -34, -26), S( -15, -21), S( -27, -17), S( -22, -20) },
        { S( -34,  -6), S( -22,   3), S( -24,   3), S( -15,  -7) },
        { S( -15,  16), S(  -2,  23), S(  15,  16), S(  27,  14) },
        { S(  -5,  25), S(  27,  21), S(  35,  25), S(  53,  12) },
        { S(   9,  29), S(   4,  32), S(  44,  30), S(  56,  29) },
        { S(  23,  23), S(  33,  26), S(  17,  26), S(  26,  21) }
    },

    // Queen
    {
        { S(  10, -80), S(   7, -89), S(  12,-103), S(  18, -73) },
        { S(   8, -76), S(  18, -80), S(  20, -72), S(  18, -55) },
        { S(  10, -50), S(  13, -29), S(   9, -11), S(   2, -11) },
        { S(   3,  -9), S(  16,  -3), S(  -0,  23), S(  -2,  39) },
        { S(  13,  -4), S(  -1,  40), S(   2,  41), S(  -4,  59) },
        { S(   5,   4), S(   3,  33), S(  -6,  68), S(  -3,  67) },
        { S( -16,  17), S( -47,  47), S(  -6,  63), S( -24,  87) },
        { S( -17,  27), S( -25,  41), S( -20,  57), S( -24,  63) }
    },

    // King
    {
        { S(  30,-101), S(  45, -52), S( -30, -38), S( -92, -25) },
        { S(  34, -52), S(  20, -20), S( -10,  -6), S( -34,  -1) },
        { S( -60, -35), S(   2, -16), S( -19,   5), S( -22,  16) },
        { S(-112, -29), S( -21,   5), S( -20,  20), S( -29,  32) },
        { S( -66,  -1), S(  19,  42), S(   7,  48), S( -20,  50) },
        { S( -25,  27), S(  58,  73), S(  43,  77), S(  31,  60) },
        { S( -38,  -1), S(  18,  73), S(  44,  66), S(  38,  54) },
        { S(  26,-244), S( 105, -28), S(  77,  -1), S(  17,  15) }
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
