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
    S(-36, 17), S(-24, 15), S(-37,  5), S(-20, -2), S(-28, 21), S( 11, 17), S( 15, 14), S(-21,-26),
    S(-34,  3), S(-41,  6), S(-21, -3), S(-20, -2), S(-12,  7), S(-25,  6), S(  3,-11), S(-21,-18),
    S(-34, 11), S(-38,  2), S(-16,-20), S( -1,-29), S(  1,-21), S( -2, -9), S(-12,-10), S(-29,-13),
    S(-19, 31), S(-27,  8), S(-18, -1), S(  1,-31), S(  7,-22), S( 19,-11), S( -7,  3), S(-25, 10),
    S(  2, 37), S(-10, 33), S( 12, -5), S( 24,-24), S( 41, -8), S( 90,  7), S( 58, 31), S( 18, 28),
    S( 88,  8), S( 63, 18), S( 60, -2), S( 83,-40), S( 95,-27), S( 42, -8), S(-74, 20), S(-68, 25)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -57, -45), S(  -3, -44), S(  -7, -20), S(   4,  -6),
    S( -11, -31), S( -15, -13), S(   9, -27), S(  14,  -5),
    S(   5, -44), S(  15, -11), S(  24,  -9), S(  33,  17),
    S(  15,  10), S(  30,  22), S(  38,  25), S(  28,  46),
    S(  33,  35), S(  23,  26), S(  44,  29), S(  30,  47),
    S( -29,  13), S(  21,  16), S(  29,  24), S(  43,  30),
    S(  -8, -16), S( -23,   6), S(  45,  -8), S(  53,  18),
    S(-167, -61), S(-109,   8), S(-105,  23), S(  31,   3)
};

const scorepair_t BishopSQT[32] = {
    S(  21, -49), S(  20, -28), S(   0, -14), S(   9, -16),
    S(  24, -31), S(  35, -35), S(  27, -14), S(  12,   3),
    S(  17,  -4), S(  30,  -1), S(  21, -13), S(  13,  30),
    S(  24, -29), S(  15,  11), S(  17,  27), S(  34,  46),
    S(  -1, -11), S(  19,  27), S(  28,  23), S(  27,  52),
    S(  31,  -9), S(  19,  28), S(  33,   7), S(  44,  17),
    S( -63,   1), S( -54, -11), S( -16,  13), S(   1,  14),
    S( -61, -21), S( -41,  13), S(-140,  19), S(-102,  17)
};

const scorepair_t RookSQT[32] = {
    S( -13, -42), S( -12, -37), S(  -5, -32), S(   2, -34),
    S( -41, -39), S( -28, -33), S(  -3, -22), S( -12, -29),
    S( -37, -21), S( -17, -19), S( -23, -10), S( -20, -11),
    S( -22,  -5), S( -22,   6), S( -32,  13), S(  -7,  -1),
    S(  -8,  20), S(   3,  28), S(  14,  25), S(  33,  12),
    S(  -8,  27), S(  24,  22), S(  37,  33), S(  62,  16),
    S(  14,  32), S(  -8,  34), S(  38,  34), S(  56,  34),
    S(  25,  28), S(  30,  33), S(  18,  39), S(  22,  28)
};

const scorepair_t QueenSQT[32] = {
    S(   6, -79), S(   8, -85), S(  15,-104), S(  23, -85),
    S(  16, -68), S(  21, -80), S(  27, -64), S(  18, -35),
    S(  17, -42), S(  22, -18), S(  13,  12), S(  14,   3),
    S(  10,   2), S(  23,   6), S(   3,  34), S(  -2,  56),
    S(  13,   5), S(  -1,  47), S(   9,  44), S(  -2,  71),
    S(   3,  15), S(  -0,  42), S(  -4,  73), S(  -3,  73),
    S( -14,  24), S( -37,  49), S( -13,  67), S( -25,  90),
    S( -16,  33), S( -16,  45), S( -14,  59), S( -18,  68)
};

const scorepair_t KingSQT[32] = {
    S(  47,-117), S(  55, -66), S( -40, -48), S( -96, -42),
    S(  37, -60), S(   7, -22), S( -18,  -8), S( -44,  -5),
    S( -67, -48), S(   3, -16), S( -15,   7), S( -10,  14),
    S(-119, -28), S( -23,   8), S( -18,  22), S( -21,  35),
    S( -71,   3), S(  17,  49), S(  10,  50), S( -11,  57),
    S( -27,  30), S(  54,  84), S(  47,  87), S(  35,  67),
    S( -40,  -5), S(  17,  74), S(  43,  70), S(  38,  60),
    S(  26,-237), S( 104, -25), S(  77,   8), S(  17,  18)
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
