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
const scorepair_t PawnSQT[48] = {
    S(-34, 19), S(-31, 16), S(-41,  8), S(-23, -2), S(-25,  7), S( 12, 15), S( 15,  6), S(-17,-23),
    S(-32,  9), S(-44, 13), S(-23,  1), S(-21, -1), S(-16,  5), S(-19,  7), S( -3,-12), S(-16,-20),
    S(-34, 15), S(-34,  9), S(-16,-15), S( -9,-27), S( -1,-20), S( -3,-13), S( -7,-14), S(-25,-18),
    S(-24, 36), S(-24, 19), S(-17, -2), S(  4,-29), S( 19,-19), S( 29,-19), S(  1, -2), S(-13,  3),
    S(-15, 54), S(-14, 39), S(  7,  6), S( 25,-27), S( 34,-25), S( 99, -2), S( 46, 20), S(  8, 19),
    S( 82, 20), S( 63, 20), S( 65, -1), S( 87,-40), S( 98,-39), S( 45,-18), S(-79, 22), S(-67, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -51, -47), S(  -2, -56), S( -16, -28), S(  -5,  -9),
    S(  -8, -27), S( -12,  -9), S(   6, -27), S(  10, -11),
    S(  -2, -40), S(   9, -14), S(  14, -11), S(  25,  15),
    S(   8,  10), S(  29,  16), S(  25,  25), S(  25,  34),
    S(  21,  23), S(  20,  21), S(  40,  26), S(  24,  42),
    S( -23,  13), S(  20,  18), S(  39,  26), S(  50,  28),
    S(   3, -12), S( -14,   6), S(  54,  -0), S(  55,  16),
    S(-162, -58), S(-109,   7), S(-103,  24), S(  28,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  13, -50), S(  26, -28), S(  -1, -20), S(  -5, -15),
    S(  20, -35), S(  21, -36), S(  24, -14), S(   5,  -2),
    S(  14, -17), S(  25,  -4), S(  13, -12), S(  14,  23),
    S(  17, -29), S(  19,   6), S(  13,  28), S(  33,  39),
    S(  -0,  -8), S(  16,  22), S(  34,  25), S(  38,  46),
    S(  23,  -5), S(  24,  25), S(  41,   2), S(  39,  20),
    S( -55,   0), S( -46,  -7), S(  -8,  16), S(  -5,  11),
    S( -58, -14), S( -43,  10), S(-137,  17), S(-101,  15)
};

const scorepair_t RookSQT[32] = {
    S( -20, -42), S( -16, -34), S(  -8, -30), S(  -3, -35),
    S( -47, -41), S( -30, -40), S( -16, -35), S( -15, -36),
    S( -38, -25), S( -17, -21), S( -29, -16), S( -25, -19),
    S( -35,  -3), S( -22,   4), S( -25,   4), S( -13,  -6),
    S( -14,  18), S(  -2,  24), S(  18,  17), S(  28,  15),
    S(  -5,  28), S(  28,  22), S(  38,  27), S(  57,  15),
    S(  14,  32), S(   4,  34), S(  48,  33), S(  59,  33),
    S(  25,  25), S(  33,  28), S(  18,  27), S(  26,  23)
};

const scorepair_t QueenSQT[32] = {
    S(  11, -81), S(   7, -92), S(  13,-106), S(  17, -70),
    S(   8, -75), S(  18, -78), S(  22, -72), S(  19, -53),
    S(  14, -49), S(  15, -30), S(  12,  -7), S(   2,  -6),
    S(   7,  -8), S(  16,  -2), S(   2,  26), S(  -2,  43),
    S(  13,  -3), S(   1,  42), S(   3,  42), S(  -6,  60),
    S(  -1,   4), S(   2,  34), S(  -4,  68), S(  -6,  65),
    S( -13,  18), S( -46,  48), S(  -8,  62), S( -25,  86),
    S( -16,  28), S( -26,  40), S( -20,  58), S( -24,  63)
};

const scorepair_t KingSQT[32] = {
    S(  27,-101), S(  42, -52), S( -38, -39), S( -98, -28),
    S(  35, -52), S(  18, -19), S(  -7,  -6), S( -29,  -3),
    S( -61, -37), S(   5, -15), S( -15,   5), S( -18,  14),
    S(-112, -32), S( -21,   4), S( -19,  20), S( -29,  30),
    S( -67,  -2), S(  19,  43), S(   7,  50), S( -19,  50),
    S( -25,  29), S(  58,  78), S(  44,  78), S(  32,  60),
    S( -38,  -1), S(  18,  74), S(  44,  68), S(  38,  54),
    S(  26,-243), S( 105, -28), S(  77,   1), S(  17,  15)
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
