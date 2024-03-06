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
    S(-35, 12), S(-32,  8), S(-47,  7), S(-29,  2), S(-30, 13), S( 13, 21), S( 16,  9), S(-22,-21),
    S(-31,  2), S(-47,  5), S(-25, -2), S(-26,  0), S(-18,  9), S(-22, 10), S( -4,-12), S(-20,-18),
    S(-32, 11), S(-33,  2), S(-17,-19), S(-10,-28), S( -1,-20), S( -3,-12), S( -8,-16), S(-28,-17),
    S(-20, 32), S(-23, 14), S(-17, -5), S(  4,-29), S( 21,-18), S( 33,-18), S(  2, -1), S(-14,  7),
    S( -9, 44), S(-11, 31), S( -2,  2), S( 26,-20), S( 44, -9), S(110,  9), S( 54, 26), S( 11, 24),
    S( 90, 14), S( 67, 17), S( 64, -1), S( 86,-37), S( 96,-26), S( 44, -7), S(-74, 21), S(-69, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -43), S(  -2, -55), S( -15, -30), S(  -2,  -8),
    S(  -9, -28), S( -15,  -7), S(   6, -29), S(  10, -11),
    S(  -2, -43), S(   9, -15), S(  16, -11), S(  29,  17),
    S(  10,  14), S(  31,  21), S(  28,  28), S(  28,  39),
    S(  22,  29), S(  23,  26), S(  44,  30), S(  27,  48),
    S( -26,  18), S(  25,  20), S(  43,  29), S(  56,  31),
    S(  -1, -12), S( -17,   8), S(  58,   1), S(  58,  22),
    S(-168, -60), S(-109,   7), S(-103,  24), S(  30,   9)
};

const scorepair_t BishopSQT[32] = {
    S(  14, -56), S(  31, -28), S(  -2, -16), S(  -2, -14),
    S(  22, -41), S(  22, -40), S(  25, -13), S(   3,   1),
    S(  16, -17), S(  27,  -3), S(  13, -13), S(  13,  28),
    S(  20, -30), S(  21,   9), S(  13,  32), S(  34,  47),
    S(  -0,  -6), S(  17,  27), S(  35,  32), S(  41,  54),
    S(  28,  -4), S(  23,  31), S(  45,   2), S(  42,  25),
    S( -56,   3), S( -48,  -8), S(  -7,  21), S(  -1,  14),
    S( -60, -19), S( -43,  14), S(-140,  21), S(-101,  19)
};

const scorepair_t RookSQT[32] = {
    S( -24, -41), S( -22, -36), S( -15, -32), S( -12, -37),
    S( -49, -40), S( -31, -40), S( -15, -35), S( -13, -35),
    S( -38, -25), S( -16, -21), S( -28, -15), S( -22, -17),
    S( -33,  -0), S( -19,   8), S( -23,   8), S(  -8,  -3),
    S( -10,  23), S(   5,  30), S(  25,  23), S(  38,  21),
    S(  -2,  36), S(  34,  29), S(  46,  34), S(  67,  21),
    S(  18,  39), S(   4,  42), S(  50,  40), S(  63,  40),
    S(  29,  32), S(  32,  37), S(  15,  36), S(  24,  31)
};

const scorepair_t QueenSQT[32] = {
    S(   8, -77), S(  -5, -88), S(   3,-104), S(  14, -84),
    S(   7, -67), S(  21, -77), S(  25, -69), S(  20, -43),
    S(  17, -41), S(  20, -19), S(  12,   3), S(   5,   4),
    S(  11,   5), S(  27,   4), S(   8,  34), S(  -2,  58),
    S(  23,   6), S(   8,  54), S(   9,  51), S(  -1,  73),
    S(   3,  18), S(   8,  48), S(   1,  79), S(  -1,  75),
    S(  -7,  27), S( -40,  53), S(  -4,  72), S( -18,  93),
    S( -12,  36), S( -17,  44), S( -12,  61), S( -17,  68)
};

const scorepair_t KingSQT[32] = {
    S(  29,-123), S(  50, -67), S( -38, -53), S(-103, -44),
    S(  32, -62), S(  13, -24), S( -13, -12), S( -36, -10),
    S( -67, -43), S(   2, -18), S( -12,   4), S(  -7,  13),
    S(-119, -29), S( -24,   7), S( -14,  25), S( -20,  36),
    S( -71,   2), S(  17,  51), S(  11,  59), S( -10,  60),
    S( -26,  29), S(  54,  84), S(  48,  87), S(  36,  70),
    S( -39,  -4), S(  18,  76), S(  44,  74), S(  38,  60),
    S(  26,-237), S( 105, -21), S(  77,   8), S(  17,  18)
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
