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
    S(-38,  9), S(-20, 12), S(-45,  5), S(-28,  4), S(-29, 16), S(  7, 21), S( 17, 14), S(-23,-19),
    S(-33, -1), S(-43,  9), S(-21, -2), S(-26, -4), S( -9,  8), S(-27,  8), S(  3, -8), S(-20,-18),
    S(-35,  7), S(-38,  3), S(-19,-20), S( -9,-25), S(  1,-21), S( -8,-11), S( -8,-13), S(-30,-15),
    S(-15, 31), S(-25, 13), S(-13, -7), S(  1,-31), S( 13,-19), S( 34,-18), S( -1, -2), S(-14, 10),
    S( -7, 44), S(-13, 31), S( -1,  2), S( 25,-20), S( 44,-11), S(107, 10), S( 52, 27), S( 11, 26),
    S( 89, 17), S( 66, 20), S( 64, -1), S( 86,-40), S( 96,-32), S( 43,-12), S(-76, 19), S(-69, 22)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -43), S(  -8, -53), S( -13, -27), S(   0,  -5),
    S(  -6, -27), S( -13,  -7), S(   7, -30), S(  15,  -8),
    S(  -5, -44), S(   9, -15), S(  22, -13), S(  25,  15),
    S(  15,  12), S(  27,  18), S(  29,  27), S(  29,  38),
    S(  21,  26), S(  22,  24), S(  42,  31), S(  27,  48),
    S( -25,  17), S(  24,  21), S(  39,  28), S(  51,  29),
    S(  -2, -14), S( -19,   5), S(  56,  -0), S(  56,  20),
    S(-167, -60), S(-109,   7), S(-104,  22), S(  30,   8)
};

const scorepair_t BishopSQT[32] = {
    S(  15, -55), S(  28, -29), S(  -9, -13), S(  -2, -14),
    S(  25, -40), S(  28, -39), S(  28, -14), S(   6,  -0),
    S(  13, -14), S(  32,  -1), S(  16, -13), S(  12,  29),
    S(  15, -30), S(  17,   6), S(  13,  32), S(  33,  44),
    S(  -1,  -6), S(  18,  24), S(  32,  30), S(  36,  51),
    S(  28,  -4), S(  24,  31), S(  41,   0), S(  41,  23),
    S( -56,   2), S( -49,  -8), S(  -7,  20), S(  -3,  13),
    S( -60, -18), S( -43,  13), S(-140,  20), S(-102,  18)
};

const scorepair_t RookSQT[32] = {
    S( -24, -40), S( -20, -34), S( -13, -27), S(  -7, -34),
    S( -46, -40), S( -29, -40), S( -17, -34), S( -12, -31),
    S( -37, -25), S( -16, -21), S( -27, -14), S( -18, -14),
    S( -34,  -1), S( -20,   7), S( -23,   5), S(  -9,  -4),
    S( -13,  20), S(   1,  29), S(  21,  20), S(  35,  18),
    S(  -3,  33), S(  32,  26), S(  42,  31), S(  62,  19),
    S(  16,  35), S(   3,  39), S(  47,  37), S(  59,  37),
    S(  28,  29), S(  31,  36), S(  16,  33), S(  23,  28)
};

const scorepair_t QueenSQT[32] = {
    S(   9, -77), S(  -5, -89), S(   6,-104), S(  24, -84),
    S(   6, -69), S(  20, -77), S(  29, -71), S(  20, -40),
    S(  16, -43), S(  18, -20), S(  11,   3), S(   9,   3),
    S(  11,   1), S(  23,   2), S(   8,  32), S(  -3,  56),
    S(  21,   3), S(   5,  50), S(   8,  49), S(  -6,  69),
    S(   3,  16), S(   4,  44), S(  -2,  76), S(  -2,  73),
    S(  -8,  25), S( -41,  50), S(  -6,  69), S( -22,  91),
    S( -15,  32), S( -19,  43), S( -14,  60), S( -18,  66)
};

const scorepair_t KingSQT[32] = {
    S(  33,-119), S(  54, -66), S( -39, -48), S(-100, -43),
    S(  35, -59), S(   9, -23), S( -13, -11), S( -41, -10),
    S( -65, -43), S(   3, -21), S( -14,   4), S( -10,  13),
    S(-118, -30), S( -24,   4), S( -15,  23), S( -22,  33),
    S( -70,   2), S(  18,  49), S(  10,  59), S( -12,  60),
    S( -26,  28), S(  55,  85), S(  47,  87), S(  35,  70),
    S( -39,  -4), S(  18,  76), S(  44,  73), S(  38,  58),
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
