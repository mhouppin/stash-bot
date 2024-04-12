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
    S(-40,  8), S(-22, 10), S(-39, 10), S(-25, -2), S(-29, 20), S( 10, 22), S( 20, 12), S(-27,-19),
    S(-36, -2), S(-42,  5), S(-18, -2), S(-21, -5), S(-11,  4), S(-26, 11), S(  4,-12), S(-23,-16),
    S(-32, 10), S(-34,  3), S(-16,-19), S( -1,-29), S(  3,-26), S( -2,-12), S(-13, -9), S(-27,-17),
    S(-20, 31), S(-30, 10), S(-15, -8), S( -0,-34), S(  7,-21), S( 25,-16), S( -9, -2), S(-15, 10),
    S(  4, 51), S(-17, 28), S(  4,  2), S( 21,-23), S( 43, -6), S(101, 11), S( 47, 27), S( 21, 37),
    S( 87, 16), S( 55,  8), S( 60, -4), S( 86,-36), S( 95,-25), S( 43, -6), S(-70, 20), S(-67, 27)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -53, -44), S(  -8, -45), S(  -6, -23), S(   9,  -3),
    S(  -5, -27), S(  -1,  -6), S(  11, -24), S(  18,  -5),
    S(   2, -42), S(  16, -13), S(  28,  -9), S(  32,  19),
    S(  13,   9), S(  19,  24), S(  40,  28), S(  34,  44),
    S(  27,  26), S(  25,  26), S(  44,  31), S(  33,  48),
    S( -29,  16), S(  23,  18), S(  29,  29), S(  44,  30),
    S(  -5, -16), S( -30,   3), S(  43,  -8), S(  49,  18),
    S(-174, -70), S(-109,   6), S(-114,  16), S(  28,   5)
};

const scorepair_t BishopSQT[32] = {
    S(  26, -49), S(  26, -24), S(  -5, -12), S(   3, -12),
    S(  32, -42), S(  39, -32), S(  31, -13), S(  13,   4),
    S(  24, -12), S(  34,  -3), S(  20,  -9), S(  18,  32),
    S(  15, -29), S(  22,  10), S(  18,  30), S(  31,  45),
    S(  -1,  -7), S(  18,  25), S(  29,  31), S(  28,  54),
    S(  36,  -2), S(  19,  32), S(  29,   7), S(  43,  21),
    S( -63,  -3), S( -65, -13), S( -10,  15), S( -10,  10),
    S( -60, -21), S( -47,  11), S(-142,  15), S(-107,  10)
};

const scorepair_t RookSQT[32] = {
    S( -10, -40), S(  -8, -32), S(  -7, -22), S(   1, -32),
    S( -38, -33), S( -23, -35), S(  -7, -23), S(  -7, -24),
    S( -33, -22), S(  -9, -19), S( -26,  -8), S( -17, -10),
    S( -28,  -5), S( -23,   6), S( -26,  10), S(  -6,  -1),
    S( -16,  16), S(  -1,  26), S(  15,  18), S(  26,  12),
    S( -13,  29), S(  26,  22), S(  29,  24), S(  56,  15),
    S(  11,  31), S(  -6,  35), S(  42,  36), S(  54,  38),
    S(  26,  32), S(  29,  33), S(  16,  35), S(  22,  29)
};

const scorepair_t QueenSQT[32] = {
    S(  10, -77), S(  -1, -86), S(  17,-104), S(  30, -89),
    S(  15, -68), S(  22, -75), S(  37, -69), S(  25, -29),
    S(  14, -45), S(  27, -22), S(  17,  13), S(  15,   7),
    S(  14,  -1), S(  26,  10), S(   5,  37), S(  -1,  61),
    S(  22,   6), S(  -3,  52), S(   9,  50), S( -10,  75),
    S(   5,  14), S(  -1,  45), S( -10,  73), S(  -3,  74),
    S( -15,  20), S( -49,  44), S( -12,  72), S( -26,  92),
    S( -30,  25), S( -19,  40), S( -14,  59), S( -17,  66)
};

const scorepair_t KingSQT[32] = {
    S(  39,-122), S(  43, -58), S( -39, -44), S( -33, -62),
    S(  35, -55), S(   3, -20), S( -20,  -7), S( -50,  -3),
    S( -70, -47), S(   4, -16), S( -18,   7), S( -16,  19),
    S(-124, -34), S( -30,   1), S( -19,  26), S( -22,  38),
    S( -73,  -0), S(  15,  49), S(  10,  58), S( -10,  59),
    S( -28,  26), S(  55,  84), S(  47,  87), S(  37,  70),
    S( -40,  -6), S(  17,  73), S(  44,  73), S(  38,  58),
    S(  26,-238), S( 105, -24), S(  77,   6), S(  17,  18)
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
