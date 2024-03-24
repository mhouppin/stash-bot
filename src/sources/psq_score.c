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
    S(-38,  8), S(-24, 10), S(-43,  4), S(-22,  6), S(-38, 17), S(  6, 18), S( 15,  6), S(-26,-27),
    S(-29,  0), S(-47,  8), S(-21,  3), S(-18, -9), S(-14,  5), S(-23, 11), S( 12,-15), S(-25,-19),
    S(-34, 23), S(-43,  1), S(-23,-16), S(  6,-27), S(  3,-23), S( -2,-12), S( -9,-11), S(-32,-15),
    S(-19, 29), S(-24,  8), S(-18, -6), S(  0,-37), S(  7,-15), S( 29,-20), S(  1,  2), S(-17, 10),
    S(-13, 46), S(-10, 34), S(  2,  3), S( 19,-21), S( 46, -9), S(106, 10), S( 53, 28), S( 15, 30),
    S( 88, 19), S( 65, 20), S( 63, -2), S( 84,-41), S( 96,-31), S( 43,-11), S(-75, 20), S(-68, 22)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -45), S( -15, -51), S( -15, -28), S(  -2, -10),
    S( -11, -31), S( -10,  -9), S(   5, -28), S(  13,  -8),
    S(   7, -40), S(  12, -13), S(  24, -10), S(  37,  13),
    S(  10,  12), S(  26,  21), S(  41,  28), S(  30,  43),
    S(  20,  27), S(  26,  28), S(  37,  31), S(  40,  53),
    S( -25,  15), S(  23,  18), S(  33,  27), S(  47,  26),
    S(  -8, -15), S( -20,   4), S(  53,  -3), S(  56,  18),
    S(-171, -62), S(-110,   7), S(-106,  22), S(  29,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  14, -54), S(  35, -27), S( -10,  -9), S(   8, -13),
    S(  27, -39), S(  37, -37), S(  34, -14), S(  16,   1),
    S(  15, -17), S(  21,  -5), S(  11,  -8), S(  17,  29),
    S(  14, -29), S(  19,  10), S(  14,  30), S(  33,  44),
    S(  -5,  -8), S(  11,  25), S(  32,  27), S(  20,  52),
    S(  31,  -3), S(  24,  29), S(  39,   2), S(  45,  23),
    S( -59,   0), S( -52, -12), S( -11,  16), S(  -4,  15),
    S( -60, -19), S( -44,  11), S(-141,  18), S(-102,  17)
};

const scorepair_t RookSQT[32] = {
    S( -25, -36), S( -17, -36), S( -10, -23), S(  -3, -34),
    S( -45, -36), S( -28, -36), S( -17, -32), S( -14, -32),
    S( -34, -24), S( -16, -17), S( -24, -18), S( -22, -17),
    S( -38,  -3), S( -22,   4), S( -26,   6), S( -13,  -6),
    S( -15,  15), S(  -1,  27), S(  20,  17), S(  33,  16),
    S(  -4,  31), S(  30,  23), S(  38,  26), S(  58,  17),
    S(  17,  36), S(   2,  41), S(  46,  41), S(  58,  37),
    S(  28,  30), S(  31,  34), S(  17,  37), S(  24,  31)
};

const scorepair_t QueenSQT[32] = {
    S(  14, -77), S(   2, -88), S(  19,-100), S(  32, -88),
    S(   8, -69), S(  20, -78), S(  38, -71), S(  24, -43),
    S(   9, -43), S(  23, -17), S(   4,   5), S(  14,   7),
    S(   8,   1), S(  12,   1), S(   3,  32), S(  -8,  56),
    S(  22,   2), S(   1,  52), S(   9,  51), S( -13,  70),
    S(   6,  15), S(   5,  44), S(  -8,  74), S(  -3,  73),
    S( -15,  22), S( -47,  49), S(  -5,  70), S( -21,  90),
    S( -20,  31), S( -18,  43), S( -14,  59), S( -19,  66)
};

const scorepair_t KingSQT[32] = {
    S(  39,-120), S(  61, -64), S( -44, -53), S(-100, -42),
    S(  37, -61), S(  10, -22), S( -12,  -7), S( -45, -10),
    S( -68, -46), S(   3, -18), S( -13,   6), S( -13,  10),
    S(-119, -30), S( -24,   8), S( -17,  20), S( -22,  35),
    S( -71,   1), S(  18,  52), S(  10,  57), S( -12,  60),
    S( -26,  29), S(  55,  82), S(  47,  88), S(  35,  70),
    S( -39,  -2), S(  18,  76), S(  44,  72), S(  38,  59),
    S(  26,-238), S( 105, -22), S(  77,   6), S(  17,  17)
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
