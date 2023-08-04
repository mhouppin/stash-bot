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
    { S(-34, 17), S(-31, 15), S(-43,  8), S(-24, -2), S(-25,  7), S( 14, 14), S( 16,  6), S(-17,-22) },
    { S(-32, 10), S(-45, 13), S(-24,  1), S(-21, -2), S(-16,  5), S(-20,  7), S( -3,-11), S(-16,-19) },
    { S(-34, 15), S(-35,  8), S(-16,-16), S(-10,-27), S( -1,-21), S( -3,-13), S( -9,-13), S(-27,-16) },
    { S(-23, 35), S(-27, 18), S(-18, -4), S(  5,-29), S( 21,-20), S( 27,-19), S( -2, -1), S(-11,  3) },
    { S(-15, 53), S(-15, 40), S(  8,  7), S( 27,-29), S( 33,-25), S(105, -2), S( 47, 21), S( 11, 23) },
    { S( 83, 20), S( 64, 22), S( 65, -0), S( 88,-40), S( 97,-39), S( 43,-19), S(-77, 23), S(-67, 25) },
    { }
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -44), S(  -2, -59), S( -19, -27), S(  -6,  -9) },
        { S(  -9, -27), S( -12, -10), S(   4, -27), S(   9, -11) },
        { S(  -3, -37), S(   8, -16), S(  14, -10), S(  26,  14) },
        { S(   8,   7), S(  33,  15), S(  25,  26), S(  29,  34) },
        { S(  21,  22), S(  20,  20), S(  46,  25), S(  25,  42) },
        { S( -26,  13), S(  19,  19), S(  41,  28), S(  49,  29) },
        { S(   6, -11), S( -15,   6), S(  56,  -0), S(  55,  15) },
        { S(-162, -58), S(-110,   6), S(-102,  24), S(  30,   8) }
    },

    // Bishop
    {
        { S(  13, -51), S(  28, -28), S(  -1, -19), S(  -8, -13) },
        { S(  18, -33), S(  21, -35), S(  24, -13), S(   4,  -2) },
        { S(  15, -17), S(  24,  -3), S(  14, -11), S(  16,  23) },
        { S(  17, -30), S(  21,   7), S(  14,  28), S(  37,  40) },
        { S(   0,  -7), S(  18,  22), S(  38,  25), S(  42,  44) },
        { S(  22,  -4), S(  26,  24), S(  40,   2), S(  41,  18) },
        { S( -58,  -0), S( -51,  -7), S(  -9,  17), S(  -6,  10) },
        { S( -59, -17), S( -43,   9), S(-138,  16), S(-100,  15) }
    },

    // Rook
    {
        { S( -20, -43), S( -16, -34), S(  -8, -30), S(  -3, -33) },
        { S( -53, -41), S( -29, -39), S( -16, -34), S( -16, -35) },
        { S( -41, -25), S( -18, -21), S( -30, -15), S( -26, -18) },
        { S( -37,  -3), S( -23,   4), S( -25,   4), S( -13,  -7) },
        { S( -16,  18), S(  -2,  24), S(  21,  15), S(  31,  16) },
        { S(  -3,  26), S(  30,  21), S(  39,  25), S(  60,  15) },
        { S(  14,  31), S(   7,  32), S(  52,  31), S(  63,  32) },
        { S(  25,  25), S(  35,  29), S(  18,  27), S(  26,  23) }
    },

    // Queen
    {
        { S(   8, -80), S(   4, -91), S(  12,-108), S(  16, -72) },
        { S(   6, -75), S(  16, -78), S(  20, -73), S(  17, -54) },
        { S(  10, -46), S(  14, -31), S(   8,  -6), S(  -2,  -5) },
        { S(   4,  -6), S(  16,  -1), S(  -2,  26), S(  -3,  44) },
        { S(  14,  -2), S(  -1,  44), S(   6,  42), S(  -4,  62) },
        { S(   6,   7), S(   4,  37), S(   1,  72), S(   1,  67) },
        { S( -12,  18), S( -48,  48), S(  -2,  64), S( -18,  87) },
        { S( -13,  29), S( -25,  40), S( -17,  58), S( -20,  64) }
    },

    // King
    {
        { S(  31,-103), S(  50, -54), S( -38, -40), S( -99, -28) },
        { S(  41, -54), S(  23, -21), S(  -7,  -8), S( -31,  -3) },
        { S( -63, -39), S(   4, -17), S( -18,   4), S( -20,  13) },
        { S(-115, -33), S( -24,   3), S( -21,  19), S( -30,  29) },
        { S( -68,  -3), S(  18,  42), S(   7,  49), S( -18,  51) },
        { S( -26,  28), S(  56,  77), S(  43,  80), S(  32,  62) },
        { S( -38,  -1), S(  16,  75), S(  42,  68), S(  37,  56) },
        { S(  27,-240), S( 106, -24), S(  77,   5), S(  17,  17) }
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
