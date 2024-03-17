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
    S(-39,  7), S(-29, 12), S(-28,  7), S(-25,  6), S(-20, 21), S(  9, 11), S(  6,  6), S(-28,-16),
    S(-41, -1), S(-42, -1), S(-20, -6), S(-19, -5), S(-12,  2), S(-19,  0), S(-10,-10), S(-25,-17),
    S(-39,  8), S(-35,  0), S(-20,-14), S(  1,-26), S(  5,-25), S( -3,-16), S(-17, -9), S(-19,-13),
    S(-28, 32), S(-25,  7), S( -9,-11), S( -5,-28), S( 24,-26), S( 28,-18), S(  3, -0), S(  0,  7),
    S( -5, 48), S(-19, 43), S(  4,  4), S( 21,-29), S( 41,-14), S(104,  6), S( 56, 41), S( 14, 44),
    S( 85, 15), S( 63, 18), S( 59,  1), S( 83,-41), S( 93,-30), S( 45, -9), S(-70, 24), S(-69, 26)
};

// Square-based piece scoring for evaluation, using a file symmetry
const scorepair_t KnightSQT[32] = {
    S( -49, -38), S(  -5, -31), S(   4, -17), S(  15,   4),
    S(  -3, -19), S(  -7,  -9), S(   8, -14), S(  17,  -6),
    S(   3, -22), S(  19, -13), S(  25,  -7), S(  34,  20),
    S(  13,  15), S(  28,  18), S(  34,  25), S(  39,  36),
    S(  19,  24), S(  17,  23), S(  42,  29), S(  39,  36),
    S( -21,  13), S(  17,  13), S(  30,  18), S(  36,  21),
    S(  -6, -18), S( -11,   6), S(  45,  -8), S(  43,   3),
    S(-170, -71), S(-111,   1), S(-105,  13), S(  26,  -2)
};

const scorepair_t BishopSQT[32] = {
    S(  16, -52), S(  40, -21), S(  11,  -2), S(  12,  -3),
    S(  26, -27), S(  22, -31), S(  32,  -8), S(  10,   5),
    S(  18, -15), S(  35,   9), S(  12,  -2), S(  17,  27),
    S(  29, -23), S(   8,   9), S(  13,  23), S(  38,  37),
    S(  -9,  -1), S(  20,  15), S(  28,  27), S(  45,  46),
    S(  29,   3), S(  20,  22), S(  33,  -2), S(  35,  13),
    S( -45,  -5), S( -48, -15), S( -11,   8), S(  -5,   4),
    S( -62, -24), S( -46,   7), S(-138,  11), S(-104,   9)
};

const scorepair_t RookSQT[32] = {
    S( -18, -23), S(  -6, -23), S(  -4, -17), S(   4, -23),
    S( -23, -23), S(  -7, -19), S(   1, -15), S(   1, -14),
    S( -20, -15), S(   1, -18), S( -11, -11), S(  -8,  -7),
    S( -28,  -0), S( -18,   2), S( -28,   2), S(  -5,   0),
    S( -12,  14), S(   1,  16), S(  10,  13), S(  15,   6),
    S(  -7,  14), S(  32,  21), S(  29,  17), S(  43,   7),
    S(  10,  18), S(   0,  32), S(  34,  34), S(  44,  25),
    S(  23,  23), S(  27,  26), S(  12,  31), S(  16,  19)
};

const scorepair_t QueenSQT[32] = {
    S(  10, -72), S(   7, -78), S(  12, -78), S(  14, -56),
    S(  20, -65), S(  29, -69), S(  30, -50), S(  29, -24),
    S(  25, -39), S(  26, -15), S(  21,   7), S(  12,  12),
    S(  18,  -1), S(  20,   5), S(   4,  27), S(   2,  53),
    S(  14,   5), S(   3,  45), S(   4,  41), S( -11,  63),
    S(  12,  15), S(  -2,  35), S(  -7,  68), S( -12,  67),
    S(   2,  24), S( -36,  44), S( -20,  62), S( -29,  86),
    S( -27,  23), S( -27,  32), S( -20,  52), S( -24,  57)
};

const scorepair_t KingSQT[32] = {
    S(  45,-116), S(  47, -66), S(   3, -42), S( -92, -31),
    S(  34, -56), S(  10, -24), S( -16, -11), S( -40,  -4),
    S( -67, -41), S(  -3, -15), S( -23,   3), S( -16,  16),
    S(-121, -31), S( -28,   3), S( -18,  27), S( -27,  41),
    S( -73,  -6), S(  15,  39), S(   8,  57), S( -14,  62),
    S( -27,  20), S(  54,  75), S(  47,  82), S(  36,  77),
    S( -40,  -4), S(  17,  70), S(  44,  70), S(  39,  58),
    S(  26,-233), S( 104, -25), S(  76,   4), S(  17,  14)
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
