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
    S(-41,  5), S(-22, 14), S(-37,  9), S(-19,  4), S(-40, 16), S(  2,  9), S( 17, 12), S(-29,-36),
    S(-27, -4), S(-47,  8), S(-19, -2), S(-13, -4), S(-12, 18), S(-19, 18), S(  8,-18), S(-28,-20),
    S(-34, 13), S(-42,  6), S(-19,-13), S(  3,-30), S(  1,-23), S( -4,-15), S(-10,-13), S(-29, -6),
    S(-27, 38), S(-24, 17), S(-20, -8), S(  9,-24), S( 14,-27), S( 25,-21), S( -8, -4), S(-19,  9),
    S( -7, 50), S(-12, 36), S( -1,  7), S( 27,-18), S( 42, -9), S(103,  7), S( 48, 25), S( 23, 30),
    S( 89, 19), S( 66, 19), S( 62, -2), S( 86,-40), S( 95,-31), S( 43,-12), S(-75, 19), S(-69, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -44), S( -35, -54), S( -11, -31), S(  -1,  -6),
    S(  -8, -28), S( -11,  -5), S(  20, -26), S(  16,  -9),
    S(  -4, -43), S(   4, -12), S(  20, -15), S(  40,  19),
    S(   8,  14), S(  30,  20), S(  36,  26), S(  40,  40),
    S(  31,  27), S(  28,  25), S(  28,  32), S(  33,  54),
    S( -27,  15), S(  17,  14), S(  36,  26), S(  48,  29),
    S(  -6, -15), S( -18,   7), S(  52,  -3), S(  53,  19),
    S(-169, -61), S(-109,   6), S(-105,  23), S(  29,   8)
};

const scorepair_t BishopSQT[32] = {
    S(  20, -53), S(  33, -28), S( -28, -20), S(   3, -12),
    S(  25, -38), S(  41, -45), S(  32, -13), S(  10,   4),
    S(  18, -18), S(  26,  -3), S(  14,  -9), S(  12,  33),
    S(  14, -31), S(  19,  10), S(  14,  30), S(  27,  43),
    S(  -2,  -8), S(   8,  26), S(  29,  29), S(  40,  55),
    S(  33,  -2), S(  20,  28), S(  40,   1), S(  38,  25),
    S( -56,   2), S( -49, -11), S( -10,  17), S(  -4,  11),
    S( -62, -19), S( -44,  12), S(-139,  19), S(-102,  16)
};

const scorepair_t RookSQT[32] = {
    S( -38, -56), S( -29, -38), S( -13, -23), S(  -1, -34),
    S( -51, -41), S( -30, -38), S( -15, -32), S( -18, -35),
    S( -41, -27), S( -10, -19), S( -25, -15), S( -24, -17),
    S( -35,  -3), S( -19,   4), S( -25,   6), S( -12,  -5),
    S( -17,  20), S(   1,  25), S(  20,  19), S(  35,  17),
    S(  -6,  30), S(  28,  27), S(  41,  28), S(  62,  17),
    S(  18,  41), S(   7,  47), S(  48,  40), S(  62,  41),
    S(  27,  32), S(  32,  35), S(  15,  37), S(  23,  29)
};

const scorepair_t QueenSQT[32] = {
    S(   9, -77), S(   3, -88), S(  15,-103), S(  29, -93),
    S(  16, -68), S(  27, -77), S(  41, -72), S(  18, -37),
    S(  13, -42), S(  18, -20), S(  -5,   2), S(   6,   3),
    S(  14,   2), S(  17,   3), S(  -4,  31), S(  -7,  56),
    S(  26,   6), S(  -4,  50), S(   6,  49), S( -12,  68),
    S(   5,  16), S(   3,  44), S(  -3,  76), S(  -5,  71),
    S( -10,  24), S( -44,  50), S(  -5,  70), S( -23,  91),
    S( -21,  31), S( -20,  42), S( -14,  60), S( -18,  66)
};

const scorepair_t KingSQT[32] = {
    S(  35,-122), S(  70, -63), S( -49, -56), S(-114, -44),
    S(  44, -55), S(  21, -15), S( -16,  -6), S( -39,  -6),
    S( -66, -46), S(   2, -19), S( -17,   1), S( -16,   8),
    S(-118, -31), S( -23,   6), S( -17,  21), S( -23,  33),
    S( -71,   1), S(  17,  48), S(  10,  58), S( -12,  60),
    S( -26,  29), S(  55,  84), S(  47,  88), S(  35,  70),
    S( -39,  -3), S(  18,  76), S(  44,  74), S(  38,  59),
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
