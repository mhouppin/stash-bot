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
    S(-37, 20), S(-32, 16), S(-43,  9), S(-25, -1), S(-27,  7), S( 12, 15), S( 16,  6), S(-20,-23),
    S(-34, 10), S(-46, 13), S(-23,  1), S(-23, -1), S(-16,  5), S(-19,  6), S( -3,-13), S(-18,-21),
    S(-35, 16), S(-34,  9), S(-16,-17), S( -9,-28), S( -0,-21), S( -2,-14), S( -7,-15), S(-26,-19),
    S(-25, 38), S(-24, 20), S(-16, -3), S(  5,-30), S( 21,-21), S( 31,-20), S(  2, -3), S(-13,  3),
    S(-15, 57), S(-13, 41), S(  7,  7), S( 26,-28), S( 37,-26), S(102, -0), S( 49, 21), S(  9, 20),
    S( 84, 22), S( 64, 21), S( 65,  1), S( 88,-39), S( 97,-37), S( 44,-16), S(-76, 21), S(-67, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -52, -46), S(  -3, -54), S( -15, -29), S(  -2,  -9),
    S(  -9, -27), S( -13,  -8), S(   6, -28), S(   9, -11),
    S(  -2, -41), S(   9, -14), S(  14, -11), S(  27,  15),
    S(   9,  11), S(  29,  18), S(  25,  26), S(  26,  35),
    S(  21,  25), S(  21,  22), S(  41,  28), S(  24,  44),
    S( -24,  14), S(  22,  19), S(  40,  27), S(  52,  29),
    S(   1, -12), S( -15,   7), S(  55,   1), S(  56,  19),
    S(-163, -58), S(-109,   7), S(-103,  24), S(  29,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  13, -53), S(  29, -28), S(  -1, -17), S(  -2, -15),
    S(  20, -38), S(  21, -38), S(  24, -14), S(   3,  -1),
    S(  15, -17), S(  26,  -4), S(  12, -13), S(  12,  25),
    S(  18, -29), S(  20,   7), S(  13,  29), S(  32,  42),
    S(   0,  -8), S(  16,  24), S(  33,  27), S(  39,  49),
    S(  26,  -4), S(  23,  27), S(  42,   1), S(  40,  22),
    S( -54,   2), S( -46,  -8), S(  -7,  18), S(  -3,  12),
    S( -59, -16), S( -43,  11), S(-138,  18), S(-101,  16)
};

const scorepair_t RookSQT[32] = {
    S( -24, -41), S( -21, -36), S( -15, -32), S( -12, -36),
    S( -47, -40), S( -31, -40), S( -16, -35), S( -14, -35),
    S( -37, -25), S( -16, -21), S( -28, -16), S( -22, -19),
    S( -33,  -2), S( -20,   6), S( -23,   6), S(  -9,  -5),
    S( -11,  19), S(   2,  27), S(  21,  19), S(  32,  17),
    S(  -3,  30), S(  30,  25), S(  41,  30), S(  60,  17),
    S(  16,  34), S(   4,  37), S(  48,  35), S(  59,  34),
    S(  26,  27), S(  32,  31), S(  17,  30), S(  25,  25)
};

const scorepair_t QueenSQT[32] = {
    S(   9, -80), S(  -3, -92), S(   2,-106), S(  12, -82),
    S(   7, -73), S(  18, -78), S(  23, -71), S(  18, -47),
    S(  16, -46), S(  18, -25), S(  10,  -4), S(   3,  -2),
    S(  10,  -3), S(  23,   0), S(   6,  28), S(  -3,  49),
    S(  20,   0), S(   6,  46), S(   6,  45), S(  -3,  65),
    S(   2,   9), S(   5,  39), S(  -2,  71), S(  -4,  68),
    S( -10,  21), S( -42,  50), S(  -7,  65), S( -23,  88),
    S( -15,  30), S( -23,  41), S( -18,  58), S( -22,  64)
};

const scorepair_t KingSQT[32] = {
    S(  27,-105), S(  48, -56), S( -35, -41), S( -99, -28),
    S(  31, -54), S(  14, -19), S( -11,  -6), S( -33,  -3),
    S( -62, -41), S(   5, -17), S( -13,   4), S( -14,  14),
    S(-114, -35), S( -21,   3), S( -17,  20), S( -27,  31),
    S( -68,  -2), S(  19,  44), S(   8,  51), S( -17,  52),
    S( -25,  29), S(  57,  80), S(  45,  80), S(  33,  62),
    S( -38,  -1), S(  18,  75), S(  44,  69), S(  38,  54),
    S(  26,-241), S( 105, -25), S(  77,   4), S(  17,  16)
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
