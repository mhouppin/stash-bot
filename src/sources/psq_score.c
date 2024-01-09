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
    S(-30, 15), S(-28, 14), S(-33, 10), S(-20,  6), S(-21, 10), S(  6, 14), S( 13,  5), S(-14,-19),
    S(-29,  7), S(-37, 12), S(-20,  5), S(-19,  3), S(-14,  7), S(-15,  7), S( -1, -8), S(-13,-16),
    S(-30, 12), S(-30,  7), S(-16, -9), S(-10,-17), S( -4,-12), S( -3, -8), S( -4,-11), S(-19,-14),
    S(-24, 27), S(-20, 15), S(-15, -0), S( -1,-20), S( 12,-12), S( 21,-14), S(  3, -3), S(-12,  2),
    S(-16, 40), S(-17, 27), S(  2,  3), S( 17,-23), S( 31,-25), S( 81, -7), S( 39, 12), S(  4, 10),
    S( 76,  5), S( 58,  8), S( 60, -9), S( 83,-43), S( 95,-41), S( 45,-18), S(-78, 16), S(-67, 17)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -49, -44), S(  -1, -44), S(  -7, -25), S(  -0,  -8),
    S(  -3, -26), S(  -8,  -6), S(   9, -25), S(   9, -12),
    S(   1, -37), S(   9, -12), S(  11, -10), S(  21,   8),
    S(   7,  12), S(  25,  14), S(  20,  17), S(  19,  25),
    S(  19,  20), S(  14,  17), S(  31,  19), S(  19,  31),
    S( -19,  14), S(  17,  15), S(  32,  19), S(  41,  21),
    S(  -2, -15), S( -15,   1), S(  44,  -7), S(  51,   9),
    S(-164, -59), S(-109,   5), S(-104,  18), S(  27,   2)
};

const scorepair_t BishopSQT[32] = {
    S(  17, -45), S(  23, -26), S(   1, -18), S(   0, -13),
    S(  20, -35), S(  19, -31), S(  21, -12), S(   6,  -5),
    S(  13, -15), S(  22,  -5), S(  12, -12), S(  12,  15),
    S(  16, -25), S(  15,   2), S(  11,  18), S(  25,  27),
    S(   2, -10), S(  11,  14), S(  26,  17), S(  28,  34),
    S(  15,  -7), S(  18,  17), S(  33,  -3), S(  31,  14),
    S( -49,   2), S( -42,  -7), S(  -8,  11), S(  -5,   8),
    S( -58, -14), S( -43,   9), S(-137,  15), S(-100,  13)
};

const scorepair_t RookSQT[32] = {
    S( -22, -37), S( -18, -31), S( -12, -27), S(  -8, -31),
    S( -38, -35), S( -27, -34), S( -16, -30), S( -16, -30),
    S( -33, -23), S( -16, -19), S( -26, -16), S( -22, -18),
    S( -32,  -6), S( -20,  -1), S( -24,  -2), S( -14,  -7),
    S( -15,   7), S(  -4,  11), S(   6,   8), S(  16,   5),
    S(  -9,  15), S(  18,  10), S(  31,  14), S(  45,   4),
    S(   8,  18), S(  -3,  21), S(  37,  20), S(  48,  19),
    S(  23,  13), S(  28,  16), S(  17,  18), S(  26,  14)
};

const scorepair_t QueenSQT[32] = {
    S(  12, -80), S(   7, -89), S(   8, -96), S(   8, -63),
    S(   5, -74), S(  13, -77), S(  14, -66), S(  12, -55),
    S(   9, -52), S(   9, -35), S(   6, -18), S(  -1, -17),
    S(   2, -19), S(   9, -14), S(  -0,   9), S(  -6,  22),
    S(   7, -12), S(  -4,  27), S(  -3,  29), S( -10,  42),
    S(   1,  -2), S(   0,  25), S(  -6,  55), S( -10,  53),
    S( -17,  12), S( -40,  38), S( -12,  48), S( -28,  70),
    S( -22,  20), S( -27,  34), S( -22,  49), S( -27,  51)
};

const scorepair_t KingSQT[32] = {
    S(  27, -76), S(  34, -37), S( -24, -26), S( -84, -15),
    S(  29, -36), S(  14, -11), S( -10,   0), S( -27,   1),
    S( -54, -24), S(   1,  -8), S( -19,   7), S( -22,  14),
    S(-110, -25), S( -21,   4), S( -20,  16), S( -29,  25),
    S( -67,  -3), S(  18,  33), S(   6,  37), S( -19,  37),
    S( -25,  24), S(  57,  64), S(  43,  59), S(  31,  45),
    S( -38,  -2), S(  18,  64), S(  43,  58), S(  37,  43),
    S(  27,-241), S( 105, -29), S(  77,  -0), S(  17,  13)
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
