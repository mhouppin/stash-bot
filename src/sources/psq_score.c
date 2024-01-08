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
    S(-34, 20), S(-31, 16), S(-40,  9), S(-21, -2), S(-25,  7), S( 11, 16), S( 17,  7), S(-17,-24),
    S(-32,  9), S(-44, 13), S(-22,  2), S(-22, -0), S(-15,  5), S(-18,  6), S( -2,-12), S(-15,-21),
    S(-34, 15), S(-34,  9), S(-17,-14), S( -9,-25), S( -2,-19), S( -3,-12), S( -7,-15), S(-24,-18),
    S(-26, 36), S(-22, 20), S(-16, -1), S(  3,-29), S( 19,-18), S( 30,-18), S(  2, -2), S(-14,  4),
    S(-14, 55), S(-15, 38), S(  6,  6), S( 24,-27), S( 32,-25), S( 98, -2), S( 44, 19), S(  7, 18),
    S( 81, 18), S( 63, 20), S( 65, -2), S( 86,-40), S( 99,-40), S( 46,-19), S(-81, 22), S(-67, 24)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -51, -48), S(  -3, -56), S( -16, -28), S(  -5,  -9),
    S(  -8, -27), S( -11,  -9), S(   8, -27), S(  11, -12),
    S(  -2, -40), S(   9, -13), S(  13, -10), S(  25,  15),
    S(   7,  10), S(  31,  16), S(  25,  25), S(  25,  34),
    S(  21,  22), S(  17,  20), S(  42,  25), S(  24,  43),
    S( -23,  12), S(  18,  18), S(  39,  26), S(  48,  28),
    S(   3, -12), S( -14,   5), S(  53,  -0), S(  55,  15),
    S(-162, -58), S(-109,   7), S(-103,  24), S(  28,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  14, -49), S(  26, -28), S(  -1, -20), S(  -5, -14),
    S(  21, -34), S(  23, -35), S(  25, -14), S(   5,  -3),
    S(  14, -16), S(  27,  -4), S(  14, -12), S(  14,  23),
    S(  17, -28), S(  19,   5), S(  13,  27), S(  33,  38),
    S(   1,  -8), S(  13,  21), S(  35,  24), S(  38,  45),
    S(  19,  -5), S(  24,  24), S(  39,   2), S(  39,  19),
    S( -55,  -1), S( -48,  -7), S(  -9,  15), S(  -5,  10),
    S( -57, -12), S( -43,  10), S(-137,  17), S(-101,  15)
};

const scorepair_t RookSQT[32] = {
    S( -22, -42), S( -17, -34), S(  -9, -30), S(  -3, -37),
    S( -47, -40), S( -30, -40), S( -16, -35), S( -16, -36),
    S( -38, -25), S( -16, -20), S( -29, -17), S( -25, -20),
    S( -35,  -3), S( -22,   4), S( -26,   3), S( -12,  -5),
    S( -14,  17), S(  -2,  23), S(  17,  16), S(  28,  14),
    S(  -5,  27), S(  28,  22), S(  38,  27), S(  58,  15),
    S(  13,  32), S(   4,  34), S(  49,  33), S(  60,  32),
    S(  25,  24), S(  33,  27), S(  18,  27), S(  27,  23)
};

const scorepair_t QueenSQT[32] = {
    S(  10, -83), S(   5, -93), S(  10,-107), S(  14, -73),
    S(   6, -77), S(  16, -80), S(  20, -74), S(  17, -57),
    S(  11, -51), S(  13, -32), S(  10,  -9), S(   0,  -9),
    S(   4, -10), S(  15,  -4), S(   1,  24), S(  -4,  39),
    S(  13,  -4), S(  -1,  41), S(   4,  42), S(  -5,  59),
    S(   6,   5), S(   5,  34), S(  -0,  70), S(  -2,  67),
    S( -13,  18), S( -47,  48), S(  -5,  63), S( -23,  88),
    S( -16,  27), S( -26,  41), S( -19,  59), S( -23,  64)
};

const scorepair_t KingSQT[32] = {
    S(  30, -99), S(  43, -51), S( -35, -38), S( -96, -28),
    S(  36, -51), S(  20, -17), S(  -8,  -4), S( -29,  -2),
    S( -62, -37), S(   4, -14), S( -18,   6), S( -21,  15),
    S(-112, -33), S( -21,   3), S( -20,  20), S( -30,  31),
    S( -66,  -2), S(  19,  43), S(   6,  50), S( -20,  50),
    S( -25,  29), S(  59,  76), S(  43,  77), S(  31,  60),
    S( -38,  -1), S(  18,  74), S(  44,  67), S(  38,  54),
    S(  26,-244), S( 105, -30), S(  77,  -1), S(  17,  14)
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
