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
    S(-32, 16), S(-28, 15), S(-39,  8), S(-23, -4), S(-24,  7), S( 15, 14), S( 15,  6), S(-16,-22),
    S(-31, 10), S(-42, 12), S(-22,  1), S(-20, -2), S(-15,  5), S(-17,  7), S( -2,-10), S(-15,-18),
    S(-33, 15), S(-33,  8), S(-16,-15), S( -9,-26), S( -1,-20), S( -4,-12), S( -8,-13), S(-27,-16),
    S(-23, 34), S(-28, 17), S(-19, -4), S(  3,-28), S( 18,-20), S( 26,-19), S( -3, -1), S(-11,  2),
    S(-15, 51), S(-15, 37), S(  7,  6), S( 25,-29), S( 30,-25), S( 98, -3), S( 44, 19), S( 10, 22),
    S( 80, 18), S( 63, 20), S( 65, -2), S( 86,-40), S( 99,-41), S( 46,-20), S(-82, 23), S(-67, 25)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -51, -48), S(  -1, -58), S( -18, -27), S(  -6,  -9),
    S(  -9, -27), S( -11, -10), S(   2, -26), S(   9, -11),
    S(  -3, -37), S(   8, -16), S(  15, -10), S(  24,  14),
    S(   8,   6), S(  30,  14), S(  23,  25), S(  28,  33),
    S(  19,  21), S(  19,  20), S(  44,  24), S(  24,  41),
    S( -23,  11), S(  18,  18), S(  39,  27), S(  48,  28),
    S(   5, -11), S( -13,   5), S(  54,   0), S(  54,  13),
    S(-161, -58), S(-109,   7), S(-103,  25), S(  28,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  11, -49), S(  26, -27), S(   1, -20), S(  -7, -14),
    S(  17, -32), S(  20, -35), S(  22, -13), S(   5,  -2),
    S(  14, -17), S(  22,  -4), S(  13, -11), S(  15,  21),
    S(  15, -29), S(  20,   6), S(  14,  27), S(  35,  37),
    S(   0,  -8), S(  18,  20), S(  37,  23), S(  40,  41),
    S(  21,  -5), S(  25,  22), S(  38,   1), S(  39,  17),
    S( -55,  -2), S( -48,  -8), S(  -9,  15), S(  -6,   9),
    S( -57, -12), S( -43,   9), S(-137,  16), S(-101,  14)
};

const scorepair_t RookSQT[32] = {
    S( -19, -42), S( -16, -33), S(  -8, -30), S(  -3, -33),
    S( -51, -41), S( -29, -39), S( -16, -34), S( -16, -34),
    S( -39, -25), S( -17, -21), S( -30, -16), S( -25, -19),
    S( -36,  -4), S( -23,   3), S( -25,   3), S( -13,  -8),
    S( -16,  16), S(  -3,  22), S(  19,  14), S(  29,  14),
    S(  -5,  24), S(  28,  19), S(  36,  23), S(  57,  13),
    S(  12,  29), S(   5,  30), S(  48,  28), S(  59,  30),
    S(  24,  24), S(  34,  27), S(  18,  24), S(  26,  21)
};

const scorepair_t QueenSQT[32] = {
    S(   9, -83), S(   5, -93), S(  12,-107), S(  18, -76),
    S(   7, -78), S(  15, -80), S(  20, -74), S(  17, -55),
    S(  10, -51), S(  13, -33), S(   8, -10), S(  -1,  -9),
    S(   3,  -9), S(  15,  -3), S(  -3,  23), S(  -2,  38),
    S(  14,  -4), S(  -1,  40), S(   4,  41), S(  -5,  58),
    S(   5,   4), S(   4,  32), S(  -2,  68), S(  -2,  66),
    S( -13,  17), S( -50,  47), S(  -5,  63), S( -24,  88),
    S( -15,  28), S( -27,  41), S( -20,  59), S( -23,  65)
};

const scorepair_t KingSQT[32] = {
    S(  29, -98), S(  47, -51), S( -37, -38), S( -96, -26),
    S(  38, -51), S(  22, -19), S(  -7,  -6), S( -30,  -2),
    S( -62, -37), S(   3, -15), S( -19,   5), S( -22,  14),
    S(-111, -31), S( -21,   4), S( -21,  20), S( -31,  30),
    S( -66,  -2), S(  19,  42), S(   6,  49), S( -21,  51),
    S( -25,  29), S(  59,  75), S(  43,  78), S(  31,  61),
    S( -38,  -1), S(  18,  74), S(  44,  67), S(  38,  55),
    S(  26,-245), S( 105, -31), S(  77,  -2), S(  17,  14)
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
