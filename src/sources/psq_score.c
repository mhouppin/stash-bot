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
    S(-43,  9), S(-22, 10), S(-45, 11), S(-28, -5), S(-34, 22), S( 11, 19), S( 22,  9), S(-27,-23),
    S(-38, -3), S(-46,  4), S(-19, -4), S(-23, -7), S(-13,  2), S(-32,  9), S(  4,-14), S(-22,-19),
    S(-34, 12), S(-35,  4), S(-17,-20), S(  0,-31), S(  4,-27), S( -3,-14), S(-16, -9), S(-27,-17),
    S(-19, 35), S(-30, 12), S(-12, -6), S(  3,-33), S( 11,-22), S( 31,-16), S( -9, -1), S(-13, 11),
    S( 10, 52), S(-14, 30), S(  8,  4), S( 24,-20), S( 46, -1), S(104, 16), S( 46, 27), S( 26, 38),
    S( 89, 11), S( 51,  4), S( 59, -4), S( 87,-32), S( 96,-18), S( 43, -2), S(-63, 22), S(-65, 26)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -44), S( -13, -45), S(  -7, -21), S(   8,  -1),
    S(  -7, -26), S(  -0,  -6), S(  10, -22), S(  18,  -3),
    S(  -1, -41), S(  14, -11), S(  29,  -8), S(  34,  24),
    S(  17,   4), S(  19,  22), S(  46,  29), S(  40,  49),
    S(  38,  19), S(  29,  19), S(  52,  31), S(  39,  53),
    S( -28,  12), S(  30,  11), S(  36,  31), S(  55,  32),
    S(  -7, -16), S( -37,   3), S(  40,  -6), S(  44,  20),
    S(-178, -75), S(-110,   6), S(-122,  10), S(  25,   3)
};

const scorepair_t BishopSQT[32] = {
    S(  30, -47), S(  27, -21), S(  -6, -10), S(   5, -12),
    S(  40, -42), S(  47, -32), S(  39, -13), S(  16,   6),
    S(  29,  -8), S(  41,  -1), S(  24,  -8), S(  22,  35),
    S(  16, -27), S(  23,   9), S(  21,  29), S(  34,  42),
    S(   1,  -3), S(  18,  22), S(  26,  27), S(  26,  49),
    S(  43,   3), S(  16,  30), S(  21,   6), S(  40,  17),
    S( -65,  -3), S( -71, -12), S( -10,  15), S( -15,  10),
    S( -58, -21), S( -50,  11), S(-146,  12), S(-112,   6)
};

const scorepair_t RookSQT[32] = {
    S(  -6, -36), S(  -4, -29), S(  -2, -18), S(   7, -29),
    S( -37, -31), S( -20, -33), S(  -4, -21), S(  -5, -23),
    S( -32, -22), S(  -6, -19), S( -25,  -7), S( -15,  -9),
    S( -26,  -6), S( -23,   5), S( -25,  10), S(  -3,  -1),
    S( -15,  16), S(  -0,  23), S(  19,  17), S(  29,  10),
    S( -15,  29), S(  26,  19), S(  27,  21), S(  57,  14),
    S(  14,  30), S(  -6,  33), S(  42,  34), S(  51,  35),
    S(  24,  32), S(  27,  36), S(  15,  38), S(  21,  34)
};

const scorepair_t QueenSQT[32] = {
    S(  12, -76), S(  -2, -84), S(  17,-104), S(  34, -88),
    S(  19, -68), S(  24, -74), S(  43, -71), S(  28, -26),
    S(  16, -45), S(  30, -22), S(  19,  19), S(  17,  10),
    S(  17,  -1), S(  29,  16), S(   5,  40), S(  -2,  65),
    S(  26,  10), S(  -3,  54), S(  10,  50), S( -12,  79),
    S(   9,  15), S(  -5,  45), S( -12,  74), S(  -4,  73),
    S( -15,  18), S( -53,  38), S( -16,  73), S( -30,  93),
    S( -37,  21), S( -19,  38), S( -15,  58), S( -15,  66)
};

const scorepair_t KingSQT[32] = {
    S(  40,-119), S(  53, -59), S( -40, -45), S( -33, -63),
    S(  39, -51), S(   6, -18), S( -17,  -6), S( -49,  -1),
    S( -73, -45), S(   6, -16), S( -20,   9), S( -18,  21),
    S(-128, -34), S( -34,   1), S( -22,  28), S( -22,  40),
    S( -75,   0), S(  13,  46), S(  10,  58), S(  -9,  59),
    S( -29,  26), S(  55,  79), S(  47,  84), S(  38,  70),
    S( -40,  -8), S(  16,  70), S(  44,  73), S(  38,  58),
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
