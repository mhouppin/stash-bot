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
    S(-39,  8), S(-22, 10), S(-38, 10), S(-24, -1), S(-29, 20), S( 10, 22), S( 18, 12), S(-25,-20),
    S(-36, -2), S(-41,  5), S(-17, -1), S(-21, -4), S(-12,  5), S(-25, 11), S(  3,-11), S(-22,-16),
    S(-33, 10), S(-34,  3), S(-16,-19), S( -1,-28), S(  2,-25), S( -2,-12), S(-13, -9), S(-27,-16),
    S(-21, 31), S(-30, 10), S(-15, -7), S( -0,-33), S(  7,-21), S( 25,-16), S( -8, -2), S(-15,  9),
    S(  0, 50), S(-17, 28), S(  2,  2), S( 21,-23), S( 43, -8), S(103, 10), S( 49, 26), S( 18, 34),
    S( 87, 17), S( 60, 13), S( 61, -3), S( 86,-38), S( 95,-28), S( 43, -9), S(-73, 19), S(-68, 25)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -44), S(  -7, -47), S(  -8, -25), S(   7,  -4),
    S(  -6, -27), S(  -6,  -6), S(   9, -26), S(  16,  -6),
    S(  -0, -42), S(  14, -14), S(  26, -11), S(  30,  17),
    S(  11,  10), S(  21,  22), S(  37,  27), S(  31,  43),
    S(  26,  27), S(  23,  25), S(  41,  30), S(  30,  47),
    S( -28,  16), S(  23,  18), S(  32,  27), S(  45,  29),
    S(  -4, -15), S( -24,   4), S(  48,  -6), S(  52,  18),
    S(-172, -65), S(-109,   6), S(-109,  19), S(  29,   6)
};

const scorepair_t BishopSQT[32] = {
    S(  23, -50), S(  27, -26), S(  -5, -12), S(   2, -12),
    S(  30, -41), S(  37, -33), S(  30, -13), S(  12,   3),
    S(  22, -13), S(  32,  -3), S(  19, -10), S(  17,  31),
    S(  14, -30), S(  20,   9), S(  16,  29), S(  29,  44),
    S(  -1,  -7), S(  17,  25), S(  29,  29), S(  27,  52),
    S(  33,  -2), S(  20,  31), S(  34,   4), S(  41,  21),
    S( -61,  -1), S( -59, -13), S( -10,  16), S(  -7,  11),
    S( -60, -20), S( -45,  12), S(-141,  17), S(-104,  13)
};

const scorepair_t RookSQT[32] = {
    S( -17, -38), S( -12, -31), S(  -8, -24), S(   1, -32),
    S( -42, -35), S( -26, -36), S(  -8, -25), S(  -7, -25),
    S( -35, -22), S( -12, -19), S( -27, -10), S( -18, -11),
    S( -31,  -4), S( -23,   6), S( -26,   8), S(  -7,  -2),
    S( -16,  16), S(  -1,  25), S(  16,  17), S(  27,  11),
    S( -10,  28), S(  27,  21), S(  33,  24), S(  58,  15),
    S(  12,  31), S(  -4,  34), S(  43,  35), S(  56,  36),
    S(  27,  31), S(  30,  33), S(  16,  35), S(  23,  29)
};

const scorepair_t QueenSQT[32] = {
    S(   9, -78), S(  -1, -87), S(  15,-103), S(  27, -88),
    S(  13, -68), S(  21, -76), S(  35, -69), S(  24, -32),
    S(  13, -45), S(  25, -22), S(  16,   9), S(  14,   6),
    S(  12,   0), S(  25,   7), S(   4,  34), S(  -2,  58),
    S(  21,   5), S(  -3,  50), S(   8,  49), S(  -9,  72),
    S(   5,  14), S(   1,  44), S(  -8,  73), S(  -3,  73),
    S( -14,  21), S( -48,  46), S( -10,  70), S( -24,  91),
    S( -23,  28), S( -19,  41), S( -14,  59), S( -18,  66)
};

const scorepair_t KingSQT[32] = {
    S(  47,-121), S(  59, -64), S( -42, -47), S(-102, -43),
    S(  44, -57), S(  13, -23), S( -17,  -9), S( -45,  -5),
    S( -68, -47), S(   4, -17), S( -16,   6), S( -14,  17),
    S(-121, -33), S( -27,   1), S( -17,  24), S( -22,  36),
    S( -72,   1), S(  16,  49), S(  10,  57), S( -11,  58),
    S( -27,  27), S(  55,  84), S(  47,  87), S(  36,  69),
    S( -40,  -5), S(  17,  74), S(  44,  73), S(  38,  58),
    S(  26,-238), S( 105, -23), S(  77,   6), S(  17,  18)
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
