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
    S(-37,  7), S(-25,  6), S(-38,  7), S(-27,  6), S(-41, 10), S(  1, 17), S( 19,  8), S(-23,-28),
    S(-28,  3), S(-51,  3), S(-19, -2), S(-16, -0), S(-11, 13), S(-29, 11), S( 10,-17), S(-28,-17),
    S(-34, 11), S(-34,  5), S(-10,-11), S(  9,-26), S(  0,-21), S(  1,-15), S(-10,-13), S(-38,-10),
    S(-32, 31), S(-26, 19), S(-11, -3), S(  9,-30), S( 11,-24), S( 24,-22), S( -2, -1), S(-12,  8),
    S(-10, 48), S(-11, 34), S( -2,  4), S( 18,-23), S( 43, -9), S(106,  8), S( 55, 29), S( 11, 28),
    S( 87, 17), S( 64, 21), S( 61, -1), S( 86,-40), S( 96,-32), S( 44,-11), S(-74, 18), S(-70, 24)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -44), S( -26, -60), S( -11, -29), S(  -1,  -8),
    S(  -6, -26), S(  -8,  -7), S(  13, -26), S(  11, -10),
    S(  -4, -46), S(  10, -13), S(  22, -14), S(  35,  18),
    S(   6,  11), S(  33,  22), S(  39,  26), S(  31,  43),
    S(  27,  29), S(  20,  25), S(  36,  29), S(  36,  57),
    S( -27,  15), S(  22,  19), S(  35,  28), S(  47,  27),
    S(  -5, -14), S( -21,   6), S(  53,  -1), S(  55,  21),
    S(-166, -59), S(-109,   6), S(-106,  22), S(  29,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  20, -52), S(  28, -29), S( -18, -16), S(   1, -15),
    S(  19, -42), S(  37, -41), S(  32, -12), S(  14,   7),
    S(  15, -17), S(  29,  -4), S(  16, -13), S(  14,  27),
    S(  16, -30), S(  20,   9), S(  20,  29), S(  31,  45),
    S(   2, -11), S(  17,  27), S(  28,  31), S(  31,  54),
    S(  32,  -2), S(  16,  27), S(  36,   2), S(  37,  26),
    S( -56,   3), S( -50,  -9), S( -12,  19), S(  -4,  14),
    S( -59, -19), S( -44,  13), S(-139,  20), S(-102,  17)
};

const scorepair_t RookSQT[32] = {
    S( -27, -49), S( -27, -29), S( -16, -26), S(  -2, -37),
    S( -42, -41), S( -28, -39), S( -11, -31), S( -18, -34),
    S( -44, -26), S( -17, -21), S( -27, -13), S( -20, -19),
    S( -39,  -4), S( -23,   4), S( -29,   3), S(  -8,  -1),
    S( -16,  19), S(   2,  31), S(  19,  19), S(  30,  12),
    S(  -4,  32), S(  31,  27), S(  40,  31), S(  61,  16),
    S(  16,  38), S(   1,  41), S(  46,  39), S(  60,  39),
    S(  30,  33), S(  33,  39), S(  18,  36), S(  24,  31)
};

const scorepair_t QueenSQT[32] = {
    S(  14, -76), S(   3, -87), S(  17,-101), S(  35, -85),
    S(   8, -68), S(  22, -74), S(  46, -73), S(  22, -38),
    S(  14, -43), S(  19, -22), S(   8,   7), S(  12,   4),
    S(   7,  -0), S(  12,  -0), S(  -0,  31), S(  -4,  58),
    S(  19,   3), S(  -4,  49), S(  -1,  47), S( -10,  68),
    S(   5,  16), S(   5,  45), S(  -9,  73), S(  -6,  72),
    S( -11,  23), S( -46,  48), S(  -7,  69), S( -21,  90),
    S( -21,  31), S( -19,  42), S( -14,  60), S( -17,  67)
};

const scorepair_t KingSQT[32] = {
    S(  38,-123), S(  67, -62), S( -38, -55), S(-101, -35),
    S(  41, -59), S(   7, -25), S( -14,  -8), S( -48, -13),
    S( -67, -48), S(   0, -19), S( -17,   5), S( -14,  13),
    S(-118, -33), S( -24,   4), S( -16,  24), S( -23,  37),
    S( -71,   0), S(  18,  50), S(  10,  59), S( -12,  63),
    S( -26,  29), S(  55,  85), S(  47,  87), S(  35,  71),
    S( -39,  -3), S(  18,  75), S(  44,  72), S(  38,  59),
    S(  26,-238), S( 105, -22), S(  77,   6), S(  17,  18)
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
