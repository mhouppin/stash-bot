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
    S(-40,  7), S(-21,  8), S(-38,  9), S(-25, -4), S(-29, 20), S( 10, 19), S( 19, 10), S(-25,-21),
    S(-36, -3), S(-42,  3), S(-18, -3), S(-21, -6), S(-12,  2), S(-26,  9), S(  3,-12), S(-21,-16),
    S(-33, 11), S(-33,  2), S(-17,-20), S( -2,-29), S(  2,-25), S( -3,-13), S(-13, -9), S(-25,-15),
    S(-21, 32), S(-30, 10), S(-15, -8), S( -0,-33), S(  7,-21), S( 25,-15), S( -9, -1), S(-14, 12),
    S(  5, 50), S(-17, 28), S(  6,  2), S( 21,-23), S( 42, -3), S( 99, 13), S( 45, 29), S( 22, 40),
    S( 87, 11), S( 51,  4), S( 58, -5), S( 86,-34), S( 95,-20), S( 43, -2), S(-65, 22), S(-65, 29)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -44), S(  -8, -44), S(  -4, -20), S(   9,  -0),
    S(  -4, -25), S(   1,  -6), S(  10, -21), S(  16,  -2),
    S(   1, -39), S(  14, -10), S(  26,  -7), S(  30,  22),
    S(  15,   5), S(  18,  22), S(  40,  28), S(  34,  47),
    S(  34,  19), S(  26,  19), S(  46,  29), S(  33,  51),
    S( -28,  12), S(  27,  12), S(  33,  29), S(  51,  30),
    S(  -7, -16), S( -36,   3), S(  39,  -7), S(  44,  19),
    S(-179, -75), S(-109,   6), S(-120,  11), S(  26,   4)
};

const scorepair_t BishopSQT[32] = {
    S(  31, -46), S(  27, -21), S(  -2, -10), S(   6, -10),
    S(  36, -41), S(  41, -30), S(  34, -12), S(  15,   6),
    S(  26,  -8), S(  36,  -1), S(  23,  -7), S(  20,  33),
    S(  17, -27), S(  22,   9), S(  19,  27), S(  29,  40),
    S(   1,  -3), S(  17,  20), S(  24,  26), S(  22,  47),
    S(  38,   1), S(  15,  29), S(  21,   6), S(  37,  16),
    S( -63,  -2), S( -69, -11), S( -10,  15), S( -14,  10),
    S( -60, -21), S( -49,  11), S(-144,  13), S(-111,   7)
};

const scorepair_t RookSQT[32] = {
    S( -10, -35), S(  -7, -28), S(  -6, -16), S(   2, -27),
    S( -36, -30), S( -20, -33), S(  -6, -19), S(  -6, -21),
    S( -32, -22), S(  -7, -18), S( -25,  -6), S( -15,  -9),
    S( -27,  -7), S( -23,   4), S( -25,   9), S(  -5,  -2),
    S( -17,  14), S(  -2,  22), S(  14,  15), S(  25,   8),
    S( -16,  27), S(  24,  18), S(  25,  20), S(  53,  12),
    S(  11,  28), S(  -8,  32), S(  39,  32), S(  49,  34),
    S(  24,  32), S(  27,  35), S(  16,  37), S(  22,  32)
};

const scorepair_t QueenSQT[32] = {
    S(  15, -75), S(   2, -84), S(  19,-102), S(  31, -86),
    S(  19, -68), S(  24, -73), S(  38, -69), S(  26, -27),
    S(  16, -45), S(  27, -24), S(  17,  16), S(  16,   7),
    S(  15,  -3), S(  25,  14), S(   5,  37), S(  -2,  61),
    S(  22,   8), S(  -4,  52), S(   8,  48), S( -12,  77),
    S(   6,  14), S(  -4,  45), S( -14,  73), S(  -4,  72),
    S( -15,  18), S( -50,  39), S( -17,  72), S( -31,  92),
    S( -36,  23), S( -19,  40), S( -14,  59), S( -15,  66)
};

const scorepair_t KingSQT[32] = {
    S(  38,-118), S(  44, -57), S( -34, -44), S( -27, -62),
    S(  34, -51), S(   3, -19), S( -18,  -6), S( -49,  -1),
    S( -72, -45), S(   5, -16), S( -19,   9), S( -18,  21),
    S(-127, -34), S( -33,   1), S( -21,  26), S( -22,  39),
    S( -75,  -0), S(  13,  46), S(  10,  57), S(  -9,  59),
    S( -28,  25), S(  55,  80), S(  47,  85), S(  37,  69),
    S( -40,  -8), S(  17,  70), S(  44,  73), S(  38,  58),
    S(  26,-238), S( 105, -26), S(  77,   6), S(  17,  18)
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
