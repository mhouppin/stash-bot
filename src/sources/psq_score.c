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
    S(-34, 12), S(-30, 10), S(-46,  8), S(-28,  2), S(-30, 13), S( 12, 20), S( 16, 10), S(-22,-21),
    S(-30,  3), S(-45,  7), S(-24, -1), S(-25,  1), S(-18,  9), S(-21, 10), S( -3,-11), S(-19,-18),
    S(-32, 11), S(-33,  4), S(-17,-18), S(-11,-27), S( -2,-21), S( -4,-13), S( -7,-15), S(-28,-16),
    S(-20, 31), S(-23, 14), S(-17, -5), S(  3,-28), S( 20,-18), S( 31,-19), S(  2, -1), S(-14,  6),
    S(-10, 44), S(-12, 32), S( -0,  3), S( 26,-20), S( 45,-11), S(108,  8), S( 53, 26), S( 10, 23),
    S( 89, 18), S( 67, 20), S( 65, -0), S( 87,-40), S( 96,-32), S( 43,-12), S(-76, 19), S(-69, 22)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -44), S(  -2, -54), S( -15, -29), S(  -2,  -8),
    S(  -9, -28), S( -14,  -7), S(   6, -29), S(  10, -11),
    S(  -1, -43), S(   9, -15), S(  15, -11), S(  28,  16),
    S(   9,  13), S(  30,  20), S(  27,  27), S(  27,  37),
    S(  22,  28), S(  22,  24), S(  42,  29), S(  26,  46),
    S( -25,  17), S(  24,  20), S(  42,  28), S(  54,  30),
    S(  -1, -12), S( -16,   8), S(  57,   1), S(  57,  21),
    S(-166, -59), S(-109,   7), S(-103,  24), S(  30,   8)
};

const scorepair_t BishopSQT[32] = {
    S(  14, -55), S(  30, -28), S(  -1, -16), S(  -2, -14),
    S(  21, -40), S(  22, -39), S(  24, -13), S(   3,  -0),
    S(  15, -17), S(  27,  -3), S(  12, -13), S(  13,  27),
    S(  19, -30), S(  20,   8), S(  13,  31), S(  33,  45),
    S(  -0,  -7), S(  17,  26), S(  34,  30), S(  40,  52),
    S(  27,  -4), S(  23,  30), S(  44,   1), S(  41,  23),
    S( -55,   3), S( -47,  -8), S(  -7,  20), S(  -2,  14),
    S( -60, -18), S( -43,  13), S(-139,  20), S(-101,  18)
};

const scorepair_t RookSQT[32] = {
    S( -24, -41), S( -21, -36), S( -15, -32), S( -12, -36),
    S( -48, -40), S( -31, -40), S( -15, -35), S( -13, -35),
    S( -38, -25), S( -16, -21), S( -28, -15), S( -22, -17),
    S( -33,  -1), S( -19,   7), S( -23,   7), S(  -8,  -3),
    S( -11,  22), S(   4,  29), S(  23,  21), S(  36,  19),
    S(  -2,  34), S(  32,  27), S(  44,  32), S(  64,  20),
    S(  17,  36), S(   4,  39), S(  49,  38), S(  61,  37),
    S(  28,  30), S(  32,  35), S(  16,  34), S(  24,  29)
};

const scorepair_t QueenSQT[32] = {
    S(   8, -78), S(  -4, -89), S(   3,-104), S(  13, -84),
    S(   7, -69), S(  20, -77), S(  24, -70), S(  19, -44),
    S(  16, -43), S(  19, -21), S(  11,   1), S(   4,   2),
    S(  11,   2), S(  25,   2), S(   7,  32), S(  -2,  55),
    S(  22,   4), S(   8,  51), S(   8,  49), S(  -2,  70),
    S(   3,  15), S(   7,  45), S(  -0,  76), S(  -2,  73),
    S(  -7,  25), S( -41,  51), S(  -5,  69), S( -20,  91),
    S( -13,  34), S( -19,  43), S( -14,  60), S( -19,  66)
};

const scorepair_t KingSQT[32] = {
    S(  29,-120), S(  49, -65), S( -37, -51), S(-101, -42),
    S(  31, -61), S(  13, -24), S( -13, -12), S( -36,  -9),
    S( -65, -43), S(   2, -19), S( -13,   3), S(  -9,  13),
    S(-117, -29), S( -23,   6), S( -15,  24), S( -22,  34),
    S( -70,   2), S(  18,  50), S(  10,  58), S( -12,  60),
    S( -26,  29), S(  55,  84), S(  47,  87), S(  35,  69),
    S( -39,  -3), S(  18,  76), S(  44,  73), S(  38,  59),
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
