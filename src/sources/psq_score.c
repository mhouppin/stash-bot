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
    S(-36, 15), S(-24, 13), S(-39,  5), S(-24, -1), S(-33, 18), S(  8, 18), S( 17, 13), S(-22,-27),
    S(-34,  2), S(-45,  6), S(-23, -2), S(-23, -1), S(-15,  6), S(-30,  6), S(  4,-12), S(-19,-18),
    S(-36, 11), S(-40,  2), S(-19,-20), S(  0,-30), S(  1,-22), S( -4, -9), S(-14,-12), S(-30,-12),
    S(-18, 32), S(-25,  9), S(-16, -2), S(  2,-31), S( 10,-22), S( 27,-14), S( -2,  1), S(-18,  8),
    S( -3, 43), S(-11, 33), S(  6,  2), S( 24,-22), S( 43, -8), S(102,  8), S( 56, 31), S( 15, 26),
    S( 88, 14), S( 65, 20), S( 63, -1), S( 85,-40), S( 96,-31), S( 43,-11), S(-76, 18), S(-68, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -55, -44), S( -11, -52), S( -11, -25), S(   1,  -7),
    S( -12, -29), S( -17, -10), S(   8, -29), S(  10,  -5),
    S(   0, -44), S(  14, -13), S(  23, -10), S(  30,  16),
    S(  12,  10), S(  29,  20), S(  37,  26), S(  27,  43),
    S(  31,  32), S(  22,  25), S(  43,  30), S(  30,  46),
    S( -26,  17), S(  23,  18), S(  35,  27), S(  47,  28),
    S(  -6, -14), S( -21,   6), S(  52,  -3), S(  54,  18),
    S(-166, -60), S(-109,   8), S(-104,  23), S(  30,   5)
};

const scorepair_t BishopSQT[32] = {
    S(  16, -53), S(  26, -28), S(  -6, -17), S(   3, -15),
    S(  22, -36), S(  37, -36), S(  27, -15), S(  10,   3),
    S(  15,  -9), S(  30,  -2), S(  18, -11), S(  10,  30),
    S(  21, -30), S(  15,   8), S(  16,  28), S(  33,  46),
    S(  -3,  -9), S(  17,  25), S(  28,  25), S(  31,  53),
    S(  33,  -5), S(  22,  29), S(  38,   5), S(  42,  21),
    S( -59,   2), S( -53, -11), S( -12,  16), S(  -1,  15),
    S( -61, -19), S( -42,  13), S(-140,  19), S(-103,  17)
};

const scorepair_t RookSQT[32] = {
    S( -20, -41), S( -18, -35), S(  -9, -29), S(  -1, -32),
    S( -46, -38), S( -29, -36), S(  -7, -25), S( -12, -31),
    S( -40, -25), S( -18, -21), S( -26, -13), S( -21, -13),
    S( -29,  -3), S( -22,   6), S( -29,   8), S(  -8,  -2),
    S( -11,  19), S(   3,  28), S(  20,  23), S(  33,  13),
    S(  -5,  29), S(  27,  22), S(  39,  31), S(  62,  17),
    S(  15,  32), S(  -6,  33), S(  44,  36), S(  59,  36),
    S(  27,  30), S(  32,  35), S(  16,  36), S(  23,  28)
};

const scorepair_t QueenSQT[32] = {
    S(   7, -78), S(   3, -87), S(  12,-104), S(  23, -84),
    S(  12, -68), S(  20, -79), S(  28, -68), S(  19, -39),
    S(  16, -43), S(  20, -21), S(  12,   8), S(  11,   4),
    S(  10,   1), S(  25,   5), S(   3,  33), S(  -4,  56),
    S(  17,   5), S(   0,  48), S(   8,  47), S(  -5,  70),
    S(   4,  15), S(   3,  43), S(  -3,  75), S(  -2,  73),
    S( -11,  24), S( -42,  49), S( -10,  68), S( -22,  90),
    S( -15,  34), S( -19,  43), S( -14,  59), S( -19,  66)
};

const scorepair_t KingSQT[32] = {
    S(  44,-120), S(  57, -67), S( -42, -50), S(-102, -43),
    S(  39, -61), S(  10, -24), S( -19,  -8), S( -42,  -7),
    S( -67, -47), S(   2, -17), S( -14,   8), S( -10,  16),
    S(-118, -30), S( -23,   7), S( -16,  23), S( -22,  35),
    S( -71,   2), S(  17,  48), S(  10,  54), S( -12,  60),
    S( -26,  30), S(  55,  85), S(  47,  88), S(  35,  68),
    S( -39,  -4), S(  18,  76), S(  44,  72), S(  38,  59),
    S(  26,-238), S( 105, -24), S(  77,   7), S(  17,  18)
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
