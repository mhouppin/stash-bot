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
    S(-36, 21), S(-33, 16), S(-41, 10), S(-23,  2), S(-25,  8), S( 11, 16), S( 17,  6), S(-17,-25),
    S(-34,  9), S(-45, 13), S(-23,  2), S(-23, -0), S(-15,  5), S(-18,  6), S( -0,-13), S(-15,-23),
    S(-35, 15), S(-35,  9), S(-17,-15), S( -9,-26), S( -2,-19), S( -2,-13), S( -5,-16), S(-24,-20),
    S(-27, 37), S(-23, 20), S(-16, -1), S(  3,-29), S( 20,-18), S( 32,-19), S(  5, -3), S(-14,  3),
    S(-16, 57), S(-15, 38), S(  5,  6), S( 21,-26), S( 39,-27), S( 97,  0), S( 45, 21), S(  6, 16),
    S( 84, 19), S( 62, 20), S( 65, -0), S( 88,-39), S( 97,-37), S( 46,-13), S(-78, 19), S(-69, 20)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -52, -46), S(  -3, -52), S( -12, -30), S(  -3,  -8),
    S(  -7, -28), S( -12,  -6), S(   9, -28), S(  11, -12),
    S(  -2, -44), S(  10, -11), S(  13, -10), S(  26,  15),
    S(   7,  16), S(  31,  19), S(  26,  25), S(  25,  35),
    S(  24,  26), S(  18,  22), S(  41,  27), S(  25,  43),
    S( -22,  15), S(  20,  18), S(  39,  25), S(  49,  28),
    S(  -4, -14), S( -15,   5), S(  50,  -1), S(  57,  19),
    S(-166, -60), S(-109,   6), S(-105,  21), S(  27,   6)
};

const scorepair_t BishopSQT[32] = {
    S(  17, -51), S(  27, -30), S(   0, -20), S(  -3, -15),
    S(  24, -41), S(  23, -35), S(  26, -14), S(   6,  -2),
    S(  14, -15), S(  28,  -4), S(  14, -12), S(  14,  24),
    S(  18, -27), S(  19,   6), S(  13,  28), S(  32,  40),
    S(   2,  -9), S(  14,  23), S(  33,  26), S(  36,  49),
    S(  18,  -5), S(  22,  27), S(  41,   2), S(  38,  22),
    S( -56,   4), S( -48,  -7), S(  -8,  17), S(  -4,  12),
    S( -59, -14), S( -44,  11), S(-139,  18), S(-102,  16)
};

const scorepair_t RookSQT[32] = {
    S( -22, -42), S( -17, -34), S(  -9, -29), S(  -3, -36),
    S( -44, -40), S( -31, -40), S( -15, -36), S( -15, -36),
    S( -38, -25), S( -15, -19), S( -29, -16), S( -24, -19),
    S( -35,  -1), S( -21,   5), S( -26,   4), S( -13,  -3),
    S( -12,  18), S(  -0,  24), S(  13,  20), S(  26,  15),
    S(  -7,  30), S(  26,  24), S(  41,  29), S(  58,  17),
    S(  16,  33), S(   1,  37), S(  48,  36), S(  61,  34),
    S(  26,  25), S(  31,  28), S(  19,  30), S(  28,  25)
};

const scorepair_t QueenSQT[32] = {
    S(  12, -81), S(   6, -93), S(  10,-105), S(  15, -67),
    S(   5, -73), S(  17, -79), S(  21, -70), S(  18, -55),
    S(  13, -50), S(  13, -27), S(  11,  -7), S(   1,  -6),
    S(   5, -10), S(  16,  -4), S(   3,  25), S(  -5,  45),
    S(  13,  -1), S(   0,  44), S(   3,  47), S(  -4,  63),
    S(   6,   9), S(   6,  39), S(   4,  75), S(  -4,  73),
    S( -14,  22), S( -41,  48), S(  -5,  64), S( -19,  88),
    S( -20,  27), S( -22,  41), S( -17,  60), S( -21,  64)
};

const scorepair_t KingSQT[32] = {
    S(  30,-101), S(  41, -51), S( -37, -37), S( -97, -28),
    S(  35, -50), S(  17, -16), S( -13,  -2), S( -30,  -1),
    S( -61, -38), S(   5, -14), S( -15,   6), S( -17,  16),
    S(-114, -36), S( -20,   2), S( -18,  20), S( -26,  31),
    S( -68,  -4), S(  18,  44), S(   8,  50), S( -17,  49),
    S( -25,  27), S(  58,  80), S(  45,  74), S(  32,  58),
    S( -38,  -2), S(  18,  73), S(  44,  67), S(  37,  49),
    S(  26,-241), S( 105, -27), S(  77,   2), S(  17,  15)
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
