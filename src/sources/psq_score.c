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
    S(-36, 15), S(-24, 13), S(-40,  4), S(-26,  0), S(-34, 16), S(  7, 18), S( 17, 12), S(-21,-27),
    S(-33,  3), S(-45,  7), S(-24, -1), S(-23,  1), S(-16,  7), S(-28,  6), S(  4,-11), S(-19,-17),
    S(-34, 11), S(-40,  2), S(-20,-20), S(  0,-29), S( -1,-22), S( -3,-10), S(-12,-11), S(-29,-12),
    S(-18, 32), S(-25, 10), S(-16, -2), S(  2,-30), S( 10,-21), S( 28,-15), S( -0,  1), S(-18,  6),
    S( -6, 43), S(-12, 33), S(  2,  2), S( 25,-21), S( 43,-10), S(104,  7), S( 55, 28), S( 13, 25),
    S( 89, 16), S( 65, 19), S( 64, -0), S( 86,-41), S( 96,-31), S( 43,-11), S(-76, 19), S(-68, 23)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -54, -44), S( -12, -53), S( -14, -27), S(  -1,  -9),
    S( -12, -29), S( -16,  -9), S(   8, -29), S(   9,  -9),
    S(  -1, -42), S(  14, -15), S(  22, -10), S(  30,  16),
    S(  14,  12), S(  29,  21), S(  36,  28), S(  25,  43),
    S(  27,  31), S(  22,  24), S(  43,  30), S(  28,  45),
    S( -26,  16), S(  24,  19), S(  38,  25), S(  49,  28),
    S(  -4, -14), S( -19,   7), S(  54,  -1), S(  55,  19),
    S(-166, -59), S(-109,   8), S(-104,  23), S(  30,   7)
};

const scorepair_t BishopSQT[32] = {
    S(  15, -54), S(  27, -28), S(  -7, -16), S(   1, -13),
    S(  21, -39), S(  37, -38), S(  26, -15), S(  10,   2),
    S(  16, -13), S(  28,  -2), S(  17, -11), S(  10,  29),
    S(  23, -29), S(  15,   8), S(  15,  29), S(  32,  45),
    S(  -2,  -9), S(  17,  26), S(  31,  27), S(  33,  52),
    S(  30,  -5), S(  23,  30), S(  40,   2), S(  40,  21),
    S( -58,   3), S( -49,  -9), S( -10,  18), S(  -1,  14),
    S( -61, -19), S( -43,  13), S(-140,  20), S(-102,  17)
};

const scorepair_t RookSQT[32] = {
    S( -21, -42), S( -21, -34), S( -11, -29), S(  -3, -31),
    S( -46, -39), S( -29, -38), S(  -8, -29), S( -13, -32),
    S( -40, -25), S( -19, -22), S( -27, -13), S( -21, -16),
    S( -31,  -2), S( -20,   6), S( -25,  10), S( -10,  -4),
    S( -11,  21), S(   2,  27), S(  20,  21), S(  34,  15),
    S(  -3,  30), S(  28,  24), S(  42,  32), S(  62,  18),
    S(  15,  32), S(  -1,  36), S(  46,  36), S(  60,  37),
    S(  28,  30), S(  32,  35), S(  16,  36), S(  23,  27)
};

const scorepair_t QueenSQT[32] = {
    S(   8, -78), S(  -2, -89), S(   8,-103), S(  23, -84),
    S(  11, -68), S(  19, -78), S(  29, -69), S(  19, -41),
    S(  16, -43), S(  20, -19), S(  11,   5), S(   9,   4),
    S(  12,   2), S(  25,   3), S(   5,  33), S(  -6,  55),
    S(  19,   4), S(   2,  49), S(   7,  49), S(  -4,  70),
    S(   4,  15), S(   6,  44), S(  -3,  74), S(  -2,  74),
    S( -10,  24), S( -42,  49), S(  -8,  67), S( -21,  90),
    S( -14,  33), S( -19,  43), S( -14,  60), S( -18,  66)
};

const scorepair_t KingSQT[32] = {
    S(  38,-120), S(  55, -67), S( -39, -52), S(-103, -42),
    S(  36, -61), S(  10, -24), S( -18,  -9), S( -40,  -9),
    S( -66, -46), S(   2, -18), S( -13,   6), S(  -9,  14),
    S(-117, -30), S( -23,   6), S( -16,  24), S( -22,  35),
    S( -71,   1), S(  18,  50), S(  10,  56), S( -12,  60),
    S( -26,  30), S(  55,  85), S(  47,  87), S(  35,  68),
    S( -39,  -3), S(  18,  76), S(  44,  72), S(  38,  59),
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
