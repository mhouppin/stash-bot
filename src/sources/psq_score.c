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
    S(-37, 15), S(-24, 13), S(-39,  3), S(-23, -3), S(-31, 18), S(  8, 18), S( 16, 14), S(-22,-27),
    S(-34,  1), S(-44,  6), S(-22, -3), S(-22, -2), S(-14,  6), S(-28,  6), S(  3,-11), S(-19,-18),
    S(-35, 11), S(-39,  2), S(-18,-21), S( -0,-30), S(  1,-22), S( -3,-10), S(-14,-11), S(-29,-12),
    S(-19, 33), S(-25,  9), S(-17, -0), S(  2,-32), S(  9,-23), S( 24,-12), S( -6,  3), S(-20,  8),
    S(  2, 41), S(-10, 35), S(  9, -1), S( 25,-22), S( 43, -8), S( 96,  8), S( 57, 31), S( 17, 28),
    S( 88, 12), S( 64, 19), S( 62, -2), S( 85,-40), S( 96,-29), S( 43, -9), S(-75, 19), S(-68, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -56, -45), S(  -8, -47), S( -11, -23), S(   1,  -8),
    S( -13, -30), S( -18, -12), S(   9, -28), S(  12,  -5),
    S(   2, -44), S(  16, -13), S(  25,  -9), S(  32,  16),
    S(  14,   9), S(  29,  20), S(  39,  25), S(  28,  45),
    S(  32,  34), S(  23,  26), S(  44,  31), S(  31,  47),
    S( -28,  15), S(  22,  17), S(  32,  26), S(  44,  29),
    S(  -7, -15), S( -22,   7), S(  49,  -6), S(  54,  18),
    S(-166, -60), S(-109,   8), S(-105,  23), S(  30,   5)
};

const scorepair_t BishopSQT[32] = {
    S(  17, -51), S(  24, -29), S(  -3, -15), S(   5, -16),
    S(  24, -34), S(  37, -36), S(  27, -14), S(  11,   3),
    S(  17,  -5), S(  32,  -2), S(  19, -11), S(  12,  30),
    S(  24, -29), S(  13,   9), S(  17,  27), S(  33,  45),
    S(  -3, -10), S(  18,  25), S(  27,  24), S(  29,  53),
    S(  34,  -6), S(  20,  28), S(  35,   6), S(  42,  18),
    S( -62,   1), S( -54, -13), S( -15,  14), S(   0,  14),
    S( -60, -19), S( -42,  13), S(-140,  19), S(-103,  16)
};

const scorepair_t RookSQT[32] = {
    S( -16, -40), S( -15, -34), S(  -6, -30), S(   1, -31),
    S( -44, -38), S( -28, -35), S(  -4, -23), S( -11, -29),
    S( -38, -23), S( -18, -21), S( -25, -10), S( -19, -13),
    S( -26,  -2), S( -22,   6), S( -31,  11), S(  -8,  -4),
    S( -10,  19), S(   2,  26), S(  17,  23), S(  33,  12),
    S(  -8,  27), S(  24,  20), S(  38,  32), S(  61,  16),
    S(  15,  31), S(  -8,  34), S(  42,  33), S(  57,  35),
    S(  27,  30), S(  31,  34), S(  17,  38), S(  22,  28)
};

const scorepair_t QueenSQT[32] = {
    S(   7, -78), S(   5, -86), S(  13,-104), S(  24, -86),
    S(  14, -68), S(  21, -78), S(  28, -66), S(  19, -36),
    S(  16, -43), S(  21, -20), S(  13,  10), S(  12,   4),
    S(  11,   2), S(  26,   5), S(   3,  34), S(  -2,  56),
    S(  16,   5), S(  -1,  48), S(   9,  46), S(  -4,  71),
    S(   3,  15), S(  -0,  42), S(  -5,  74), S(  -3,  72),
    S( -13,  24), S( -41,  48), S( -12,  68), S( -23,  91),
    S( -16,  33), S( -18,  43), S( -15,  58), S( -18,  67)
};

const scorepair_t KingSQT[32] = {
    S(  48,-121), S(  57, -67), S( -42, -49), S(-100, -42),
    S(  41, -61), S(  11, -23), S( -21,  -7), S( -44,  -5),
    S( -68, -48), S(   2, -16), S( -16,   7), S( -10,  15),
    S(-119, -31), S( -24,   6), S( -17,  23), S( -22,  36),
    S( -71,   1), S(  17,  49), S(  10,  52), S( -11,  58),
    S( -27,  29), S(  55,  84), S(  47,  89), S(  35,  67),
    S( -39,  -4), S(  18,  75), S(  44,  71), S(  38,  60),
    S(  26,-237), S( 105, -24), S(  77,   7), S(  17,  18)
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
