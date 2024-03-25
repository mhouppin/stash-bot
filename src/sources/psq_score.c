/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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
const scorepair_t PawnSQT[48] = {
    S(-35, 15), S(-24, 11), S(-43,  6), S(-26,  2), S(-33, 13), S(  7, 19), S( 16, 12), S(-22,-27),
    S(-34,  2), S(-43,  8), S(-24, -0), S(-25,  1), S(-15,  8), S(-26,  7), S(  3,-11), S(-17,-18),
    S(-34, 11), S(-41,  4), S(-21,-18), S( -0,-31), S( -1,-20), S( -3,-10), S(-11,-13), S(-29,-12),
    S(-21, 32), S(-23, 12), S(-15, -3), S(  4,-31), S( 13,-20), S( 31,-17), S( -0, -0), S(-16,  5),
    S( -8, 45), S(-12, 33), S( -1,  2), S( 26,-21), S( 43,-10), S(106,  7), S( 54, 27), S( 12, 25),
    S( 89, 17), S( 66, 20), S( 65,  0), S( 86,-41), S( 97,-32), S( 43,-12), S(-76, 20), S(-69, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -44), S( -13, -54), S( -14, -28), S(  -1,  -8),
    S( -11, -28), S( -15,  -8), S(  10, -29), S(  10,  -7),
    S(  -2, -43), S(   8, -16), S(  23, -11), S(  29,  16),
    S(  10,  11), S(  28,  19), S(  39,  28), S(  28,  42),
    S(  25,  30), S(  20,  22), S(  43,  30), S(  26,  46),
    S( -25,  17), S(  23,  19), S(  38,  26), S(  51,  29),
    S(  -2, -12), S( -16,   9), S(  56,   1), S(  56,  20),
    S(-166, -59), S(-109,   7), S(-103,  24), S(  30,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  13, -56), S(  28, -29), S(  -7, -17), S(   1, -14),
    S(  19, -39), S(  33, -39), S(  22, -12), S(   9,   4),
    S(  15, -16), S(  30,  -3), S(  17, -12), S(  11,  28),
    S(  19, -30), S(  17,   8), S(  13,  29), S(  34,  44),
    S(  -0,  -7), S(  19,  26), S(  32,  28), S(  35,  53),
    S(  29,  -4), S(  21,  29), S(  44,   4), S(  39,  21),
    S( -56,   3), S( -49,  -9), S(  -9,  19), S(  -1,  14),
    S( -59, -18), S( -43,  13), S(-139,  20), S(-101,  18)
};

const scorepair_t RookSQT[32] = {
    S( -23, -39), S( -22, -37), S( -12, -29), S(  -3, -30),
    S( -46, -41), S( -30, -39), S( -14, -33), S( -13, -34),
    S( -38, -24), S( -18, -22), S( -28, -14), S( -23, -16),
    S( -32,  -2), S( -21,   7), S( -25,   7), S(  -8,  -4),
    S(  -9,  22), S(   5,  28), S(  20,  20), S(  35,  17),
    S(  -4,  33), S(  30,  25), S(  43,  31), S(  63,  20),
    S(  16,  34), S(   3,  37), S(  48,  38), S(  59,  36),
    S(  28,  31), S(  32,  35), S(  16,  34), S(  24,  28)
};

const scorepair_t QueenSQT[32] = {
    S(   8, -78), S(  -4, -89), S(   6,-105), S(  22, -82),
    S(  10, -68), S(  22, -78), S(  26, -69), S(  15, -42),
    S(  15, -44), S(  18, -21), S(   9,   2), S(   9,   3),
    S(   8,   2), S(  24,   2), S(   9,  33), S(  -3,  55),
    S(  20,   4), S(   5,  50), S(   9,  49), S(  -3,  70),
    S(   2,  15), S(   5,  44), S(   0,  75), S(  -2,  73),
    S( -10,  23), S( -44,  50), S(  -6,  69), S( -21,  91),
    S( -13,  34), S( -18,  44), S( -14,  60), S( -18,  67)
};

const scorepair_t KingSQT[32] = {
    S(  35,-120), S(  53, -65), S( -37, -51), S(-104, -42),
    S(  34, -60), S(   9, -24), S( -15, -12), S( -37,  -7),
    S( -66, -43), S(   3, -18), S( -13,   3), S( -10,  12),
    S(-118, -30), S( -23,   6), S( -16,  22), S( -22,  35),
    S( -70,   3), S(  18,  51), S(  10,  57), S( -12,  60),
    S( -26,  29), S(  55,  84), S(  47,  88), S(  35,  69),
    S( -39,  -3), S(  18,  75), S(  44,  73), S(  38,  59),
    S(  26,-238), S( 105, -22), S(  77,   7), S(  17,  18)
};

#undef S

// clang-format on

static void psq_score_init_piece(const scorepair_t *table, piece_t piece)
{
    const scorepair_t pieceValue =
        create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        file_t queensideFile = imin(sq_file(square), sq_file(square) ^ 7);
        scorepair_t psqEntry = pieceValue + table[sq_rank(square) * 4 + queensideFile];

        PsqScore[piece][square] = psqEntry;
        PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
    }
}

static void psq_score_init_pawn(void)
{
    const scorepair_t pieceValue = create_scorepair(PAWN_MG_SCORE, PAWN_EG_SCORE);

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        if (sq_rank(square) == RANK_1 || sq_rank(square) == RANK_8)
        {
            PsqScore[WHITE_PAWN][square] = 0;
            PsqScore[BLACK_PAWN][opposite_sq(square)] = 0;
        }
        else
        {
            scorepair_t psqEntry = pieceValue + PawnSQT[square - SQ_A2];

            PsqScore[WHITE_PAWN][square] = psqEntry;
            PsqScore[BLACK_PAWN][opposite_sq(square)] = -psqEntry;
        }
    }
}

void psq_score_init(void)
{
    psq_score_init_pawn();
    psq_score_init_piece(KnightSQT, KNIGHT);
    psq_score_init_piece(BishopSQT, BISHOP);
    psq_score_init_piece(RookSQT, ROOK);
    psq_score_init_piece(QueenSQT, QUEEN);
    psq_score_init_piece(KingSQT, KING);
}
