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
    S(-36, 14), S(-24, 13), S(-42,  6), S(-26,  1), S(-36, 15), S(  8, 17), S( 16, 10), S(-21,-28),
    S(-33,  2), S(-43,  7), S(-22, -2), S(-23,  1), S(-16,  9), S(-28,  6), S(  3,-10), S(-18,-17),
    S(-36, 11), S(-40,  4), S(-22,-19), S( -0,-27), S( -0,-21), S( -3,-11), S(-10,-12), S(-29,-12),
    S(-19, 32), S(-24, 10), S(-17, -4), S(  4,-29), S( 11,-20), S( 30,-17), S(  1,  1), S(-16,  8),
    S( -7, 42), S(-12, 31), S(  2,  2), S( 26,-20), S( 43,-10), S(106,  8), S( 54, 27), S( 11, 24),
    S( 88, 17), S( 66, 19), S( 64, -1), S( 86,-41), S( 96,-31), S( 43,-12), S(-76, 19), S(-69, 22)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -44), S( -11, -53), S( -13, -27), S(  -4,  -9),
    S( -10, -28), S( -16,  -8), S(   6, -30), S(  10, -10),
    S(  -1, -45), S(  10, -15), S(  22, -11), S(  31,  17),
    S(  12,  12), S(  31,  20), S(  34,  29), S(  28,  42),
    S(  27,  30), S(  20,  25), S(  41,  28), S(  29,  46),
    S( -25,  16), S(  22,  18), S(  39,  27), S(  50,  29),
    S(  -3, -13), S( -18,   7), S(  56,  -0), S(  56,  21),
    S(-165, -59), S(-109,   7), S(-104,  23), S(  30,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  15, -55), S(  28, -28), S(  -7, -16), S(  -0, -14),
    S(  21, -38), S(  37, -38), S(  27, -14), S(  10,   2),
    S(  15, -14), S(  28,  -2), S(  16, -12), S(   9,  28),
    S(  21, -30), S(  17,   7), S(  15,  29), S(  32,  47),
    S(  -0,  -8), S(  14,  25), S(  31,  27), S(  35,  52),
    S(  29,  -3), S(  23,  30), S(  42,   3), S(  39,  21),
    S( -58,   2), S( -48,  -8), S( -10,  18), S(  -2,  14),
    S( -60, -18), S( -43,  13), S(-139,  20), S(-102,  18)
};

const scorepair_t RookSQT[32] = {
    S( -23, -42), S( -19, -34), S( -11, -31), S(  -5, -33),
    S( -47, -39), S( -31, -38), S( -10, -31), S( -11, -32),
    S( -40, -25), S( -19, -22), S( -27, -13), S( -22, -16),
    S( -33,  -2), S( -20,   6), S( -27,   7), S( -10,  -5),
    S( -12,  20), S(   3,  27), S(  22,  22), S(  35,  15),
    S(  -2,  34), S(  30,  27), S(  43,  31), S(  64,  20),
    S(  16,  32), S(   0,  37), S(  46,  36), S(  59,  36),
    S(  28,  30), S(  32,  35), S(  16,  35), S(  24,  29)
};

const scorepair_t QueenSQT[32] = {
    S(   7, -78), S(  -3, -89), S(   8,-103), S(  23, -83),
    S(   8, -69), S(  20, -77), S(  28, -69), S(  18, -42),
    S(  14, -44), S(  20, -21), S(  13,   3), S(   9,   4),
    S(   9,   2), S(  23,   3), S(   2,  32), S(  -3,  55),
    S(  20,   4), S(   7,  51), S(   8,  48), S(  -2,  70),
    S(   5,  16), S(   6,  44), S(  -1,  76), S(  -2,  72),
    S(  -9,  25), S( -44,  50), S(  -7,  69), S( -21,  91),
    S( -14,  34), S( -19,  43), S( -14,  60), S( -19,  66)
};

const scorepair_t KingSQT[32] = {
    S(  36,-118), S(  54, -67), S( -40, -52), S(-103, -43),
    S(  35, -60), S(  11, -25), S( -16, -10), S( -40,  -8),
    S( -65, -43), S(   2, -17), S( -14,   5), S( -10,  13),
    S(-118, -31), S( -23,   5), S( -16,  23), S( -22,  35),
    S( -70,   3), S(  18,  50), S(  10,  57), S( -12,  59),
    S( -26,  30), S(  55,  84), S(  47,  88), S(  35,  68),
    S( -39,  -3), S(  18,  76), S(  44,  73), S(  38,  60),
    S(  26,-238), S( 105, -23), S(  77,   7), S(  17,  18)
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
