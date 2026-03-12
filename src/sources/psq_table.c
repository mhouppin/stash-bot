/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#include "psq_table.h"

Scorepair PsqTable[PIECE_NB][SQUARE_NB];

// clang-format off

const Score PieceScores[PHASE_NB][PIECE_NB] = {
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
const Scorepair PawnSQT[48] = {
    S(-34,  10), S(-17,   4), S(-38,   9), S(-31,  -5), S(-19,   5), S(  7,  11), S( 16,   4), S(-24, -21),
    S(-34,  -2), S(-44,  -3), S(-27,  -5), S(-22,  -8), S(-11,  -6), S(-22,  -1), S(  3, -15), S(-21, -19),
    S(-28,   8), S(-35,   4), S(-14, -15), S( -8, -22), S( -1, -29), S(  1,  -9), S(-13, -10), S(-20, -14),
    S(-15,  29), S(-17,  10), S( -9,  -7), S( -2, -24), S( 11, -20), S( 21,  -9), S( -4,   2), S(-14,  14),
    S( -3,  39), S(-15,  29), S( -6,   3), S( 24,  -3), S( 43,   8), S( 81,  26), S( 33,  22), S( 11,  38),
    S(111,   2), S( 28, -16), S( 52, -14), S( 88, -12), S( 77,  -8), S( 49,  21), S(-27,  11), S(-35,  12)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -68,  -34), S(  -9,  -45), S( -23,  -10), S( -13,   10),
    S(  -6,  -19), S(  -2,    4), S(   8,   -9), S(   6,   -3),
    S(   8,  -22), S(  13,   -2), S(  24,  -12), S(  21,   16),
    S(  16,   -3), S(  18,   15), S(  45,   24), S(  36,   27),
    S(  45,   18), S(  54,   12), S(  57,   26), S(  50,   38),
    S(  -1,    8), S(  28,   14), S(  63,   27), S(  55,   26),
    S(  -3,    0), S( -35,   18), S(  33,   12), S(  42,   35),
    S(-171, -115), S(-109,   19), S(-157,   33), S(  -1,   24)
};

const Scorepair BishopSQT[32] = {
    S(  13,  -44), S(  24,  -14), S(   5,   -6), S(   5,   -8),
    S(  32,  -33), S(  37,  -31), S(  33,  -12), S(  13,   -1),
    S(  25,   -6), S(  36,    6), S(  22,   -5), S(  22,   20),
    S(   5,  -34), S(  24,    7), S(  25,   17), S(  29,   25),
    S(  13,    5), S(  16,   15), S(  38,   24), S(  30,   37),
    S(  42,    5), S(  21,   18), S(  29,   15), S(  43,   15),
    S( -30,   14), S( -51,    5), S(   5,   18), S(   9,    7),
    S( -25,    4), S( -71,   21), S(-159,   25), S(-136,   15)
};

const Scorepair RookSQT[32] = {
    S(  -9,  -28), S(   0,  -25), S(  -9,  -23), S(  -4,  -18),
    S( -30,  -28), S( -19,  -22), S( -10,  -18), S(  -7,  -14),
    S( -24,  -19), S(   8,  -10), S( -18,   -3), S( -16,    1),
    S( -21,   -5), S(  -6,    9), S( -24,   11), S(  -6,    7),
    S(   0,   21), S(  20,   22), S(  22,    9), S(  26,   11),
    S( -10,   29), S(  30,    9), S(  36,   13), S(  55,    8),
    S(   8,   32), S(  -2,   24), S(  28,   21), S(  44,   29),
    S(  -1,   38), S(  -3,   39), S(   3,   49), S(   3,   34)
};

const Scorepair QueenSQT[32] = {
    S(   8,  -75), S(   5,  -69), S(  18,  -89), S(  23,  -62),
    S(  15,  -71), S(  12,  -55), S(  32,  -52), S(  19,   -3),
    S(  12,  -37), S(  26,   -7), S(  17,   21), S(  11,   19),
    S(  13,   -4), S(  26,   19), S(   3,   41), S(  -4,   58),
    S(  30,   19), S(  -5,   55), S(   1,   30), S( -14,   61),
    S(   0,    4), S(   2,   38), S( -16,   54), S(  -4,   50),
    S(  -8,    2), S( -48,    9), S( -32,   54), S( -25,   84),
    S( -46,   25), S(  -1,   24), S( -21,   44), S( -10,   45)
};

const Scorepair KingSQT[32] = {
    S(  59, -117), S(  62,  -62), S( -13,  -41), S( -15,  -56),
    S(  60,  -58), S(  16,  -20), S(  -7,  -13), S( -36,   -5),
    S( -35,  -40), S(  -1,  -18), S( -24,    7), S(  -9,   20),
    S(-156,  -20), S( -79,   11), S( -40,   29), S(   8,   39),
    S(-104,   22), S( -21,   57), S(  24,   54), S(  46,   63),
    S( -53,   46), S(  53,   80), S(  69,   87), S(  74,   73),
    S( -68,  -15), S(   4,   73), S(  20,   84), S(  55,   63),
    S(  27, -214), S(  77,  -25), S(  55,   28), S( -26,   17)
};

#undef S

// clang-format on

static void psq_table_init_piece(const Scorepair *table, Piece piece) {
    const Scorepair piece_base =
        create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

    for (Square square = SQ_A1; square <= SQ_H8; ++square) {
        File qsfile = file_to_queenside(square_file(square));
        Scorepair psqt_entry = piece_base + table[square_rank(square) * 4 + qsfile];

        PsqTable[piece][square] = psqt_entry;
        PsqTable[opposite_piece(piece)][square_flip(square)] = -psqt_entry;
    }
}

static void psq_table_init_pawn(void) {
    const Scorepair pawn_base = create_scorepair(PAWN_MG_SCORE, PAWN_EG_SCORE);

    for (Square square = SQ_A1; square <= SQ_H8; ++square) {
        if (square_rank(square) == RANK_1 || square_rank(square) == RANK_8) {
            PsqTable[WHITE_PAWN][square] = 0;
            PsqTable[BLACK_PAWN][square_flip(square)] = 0;
        } else {
            Scorepair psqt_entry = pawn_base + PawnSQT[square - SQ_A2];

            PsqTable[WHITE_PAWN][square] = psqt_entry;
            PsqTable[BLACK_PAWN][square_flip(square)] = -psqt_entry;
        }
    }
}

void psq_table_init(void) {
    psq_table_init_pawn();
    psq_table_init_piece(KnightSQT, KNIGHT);
    psq_table_init_piece(BishopSQT, BISHOP);
    psq_table_init_piece(RookSQT, ROOK);
    psq_table_init_piece(QueenSQT, QUEEN);
    psq_table_init_piece(KingSQT, KING);
}
