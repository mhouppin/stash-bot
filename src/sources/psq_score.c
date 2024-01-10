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
    S(-33, 19), S(-28, 16), S(-34, 11), S(-19, -2), S(-20,  8), S( 12, 17), S( 15,  6), S(-15,-23),
    S(-32,  9), S(-40, 13), S(-20,  3), S(-21,  1), S(-13,  6), S(-14,  7), S(  0,-11), S(-14,-20),
    S(-34, 15), S(-32,  9), S(-16,-13), S( -9,-24), S( -2,-17), S( -3,-12), S( -5,-14), S(-24,-17),
    S(-28, 34), S(-25, 17), S(-18, -2), S( -0,-28), S( 14,-18), S( 26,-18), S(  1, -3), S(-15,  3),
    S(-18, 52), S(-18, 35), S(  3,  4), S( 21,-28), S( 32,-27), S( 94, -3), S( 43, 20), S(  4, 18),
    S( 80, 18), S( 62, 18), S( 64, -4), S( 86,-41), S( 98,-41), S( 46,-19), S(-81, 21), S(-67, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -51, -47), S(   1, -54), S( -12, -27), S(  -3,  -8),
    S(  -6, -27), S( -10,  -8), S(   7, -27), S(  12, -12),
    S(   0, -41), S(   9, -12), S(  14, -11), S(  22,  14),
    S(   7,  12), S(  28,  16), S(  23,  24), S(  23,  33),
    S(  20,  22), S(  16,  20), S(  38,  25), S(  22,  41),
    S( -22,  12), S(  18,  17), S(  38,  25), S(  47,  28),
    S(   1, -13), S( -14,   4), S(  51,  -2), S(  55,  16),
    S(-163, -59), S(-109,   7), S(-103,  23), S(  28,   6)
};

const scorepair_t BishopSQT[32] = {
    S(  16, -48), S(  24, -28), S(   4, -20), S(  -3, -13),
    S(  21, -35), S(  22, -33), S(  22, -13), S(   7,  -3),
    S(  14, -15), S(  25,  -4), S(  14, -12), S(  14,  21),
    S(  16, -27), S(  18,   5), S(  14,  25), S(  30,  36),
    S(   2,  -8), S(  14,  20), S(  32,  23), S(  33,  44),
    S(  18,  -6), S(  23,  23), S(  39,   1), S(  38,  19),
    S( -54,   1), S( -47,  -7), S(  -9,  15), S(  -5,  10),
    S( -58, -13), S( -43,  10), S(-137,  17), S(-101,  15)
};

const scorepair_t RookSQT[32] = {
    S( -20, -41), S( -17, -32), S(  -9, -29), S(  -5, -34),
    S( -44, -38), S( -29, -38), S( -15, -34), S( -15, -34),
    S( -36, -25), S( -15, -19), S( -28, -16), S( -23, -19),
    S( -33,  -3), S( -22,   4), S( -25,   2), S( -13,  -5),
    S( -15,  15), S(  -3,  21), S(  13,  14), S(  24,  12),
    S(  -7,  26), S(  25,  20), S(  36,  25), S(  55,  13),
    S(  12,  30), S(   2,  32), S(  46,  31), S(  57,  30),
    S(  24,  24), S(  32,  26), S(  18,  26), S(  26,  21)
};

const scorepair_t QueenSQT[32] = {
    S(  12, -82), S(   7, -92), S(  12,-104), S(  17, -71),
    S(   6, -76), S(  16, -79), S(  20, -71), S(  17, -55),
    S(  11, -51), S(  12, -31), S(  10, -10), S(   2, -10),
    S(   4, -10), S(  14,  -5), S(   1,  22), S(  -3,  37),
    S(  12,  -5), S(  -2,  40), S(   2,  41), S(  -6,  58),
    S(   4,   5), S(   4,  33), S(  -2,  69), S(  -4,  67),
    S( -15,  18), S( -46,  48), S(  -7,  62), S( -23,  87),
    S( -17,  27), S( -26,  41), S( -19,  59), S( -23,  64)
};

const scorepair_t KingSQT[32] = {
    S(  30, -98), S(  39, -50), S( -29, -37), S( -93, -27),
    S(  34, -50), S(  18, -18), S( -10,  -4), S( -29,  -2),
    S( -61, -35), S(   4, -14), S( -18,   5), S( -21,  15),
    S(-112, -33), S( -21,   3), S( -20,  19), S( -30,  30),
    S( -67,  -3), S(  19,  42), S(   6,  49), S( -20,  49),
    S( -25,  28), S(  59,  76), S(  43,  76), S(  31,  59),
    S( -38,  -1), S(  18,  73), S(  44,  66), S(  38,  53),
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
