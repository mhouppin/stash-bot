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
    S(-39,   6), S(-20,   8), S(-36,   6), S(-23,  -6), S(-28,  17), S(  9,  19), S( 18,  10), S(-23, -24),
    S(-37,  -4), S(-39,   2), S(-20,  -6), S(-21,  -9), S(-12,   0), S(-25,   8), S(  1, -14), S(-19, -19),
    S(-32,  11), S(-32,   2), S(-17, -22), S( -3, -31), S(  2, -28), S( -3, -14), S(-15,  -8), S(-24, -17),
    S(-19,  34), S(-28,  11), S(-13,  -8), S(  0, -34), S(  5, -22), S( 23, -13), S(-12,   2), S(-13,  15),
    S(  9,  54), S(-13,  29), S( 11,   2), S( 19, -22), S( 38,   1), S( 86,  19), S( 35,  34), S( 22,  45),
    S( 90,   5), S( 41,  -3), S( 53,  -8), S( 82, -29), S( 93,  -9), S( 41,   7), S(-52,  23), S(-59,  30)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -50,  -40), S( -14,  -40), S( -12,  -19), S(  -7,   -1),
    S(  -8,  -23), S(  -3,   -7), S(   5,  -19), S(   3,   -6),
    S(   5,  -35), S(   9,   -8), S(  17,   -9), S(  13,   20),
    S(  16,    3), S(  13,   22), S(  34,   28), S(  22,   42),
    S(  49,   14), S(  38,   18), S(  55,   28), S(  46,   49),
    S( -14,    9), S(  42,    8), S(  48,   30), S(  62,   30),
    S(  -1,   -8), S( -39,   12), S(  38,    2), S(  42,   30),
    S(-172,  -84), S(-111,    7), S(-139,   10), S(  22,    6)
};

const Scorepair BishopSQT[32] = {
    S(  33,  -40), S(  27,  -14), S(  -3,  -11), S(   2,  -12),
    S(  36,  -41), S(  39,  -28), S(  34,  -13), S(  12,    2),
    S(  28,   -7), S(  34,    0), S(  21,   -8), S(  17,   29),
    S(  16,  -25), S(  20,    7), S(  17,   23), S(  24,   35),
    S(   6,   -1), S(  22,   17), S(  26,   23), S(  27,   42),
    S(  41,    6), S(  16,   30), S(  15,    8), S(  35,   14),
    S( -60,    4), S( -67,   -5), S(  -6,   16), S( -19,   13),
    S( -54,  -19), S( -57,   15), S(-151,   15), S(-122,    8)
};

const Scorepair RookSQT[32] = {
    S(  -8,  -37), S( -10,  -25), S( -10,  -14), S(  -5,  -23),
    S( -33,  -31), S( -20,  -33), S( -10,  -17), S( -13,  -19),
    S( -30,  -25), S(  -7,  -19), S( -27,   -5), S( -20,   -6),
    S( -25,   -9), S( -23,    5), S( -27,   11), S( -11,    1),
    S( -13,   12), S(  -1,   22), S(  15,   15), S(  26,    8),
    S( -17,   27), S(  23,   18), S(  22,   21), S(  53,   12),
    S(  12,   28), S(  -7,   31), S(  37,   32), S(  45,   35),
    S(  23,   31), S(  22,   36), S(  14,   38), S(  21,   33)
};

const Scorepair QueenSQT[32] = {
    S(  15,  -76), S(   0,  -78), S(  17, -101), S(  26,  -82),
    S(  21,  -69), S(  21,  -70), S(  35,  -67), S(  20,  -21),
    S(  16,  -49), S(  26,  -25), S(  14,   20), S(  11,   12),
    S(  16,   -9), S(  20,   22), S(   2,   40), S(  -6,   66),
    S(  23,    7), S(  -3,   52), S(   9,   45), S( -13,   80),
    S(  10,    9), S(  -6,   43), S( -12,   72), S(  -2,   70),
    S( -12,   10), S( -43,   25), S( -20,   74), S( -36,   94),
    S( -50,   19), S( -18,   35), S( -15,   57), S( -12,   66)
};

const Scorepair KingSQT[32] = {
    S(  63, -122), S(  59,  -59), S( -28,  -46), S( -34,  -61),
    S(  56,  -53), S(  15,  -19), S( -19,   -6), S( -56,    0),
    S( -70,  -44), S(   8,  -17), S( -27,   10), S( -30,   22),
    S(-139,  -32), S( -46,    3), S( -30,   29), S( -23,   42),
    S( -81,    2), S(   7,   48), S(  13,   61), S(  -5,   63),
    S( -32,   25), S(  55,   77), S(  49,   86), S(  41,   72),
    S( -43,  -12), S(  14,   64), S(  43,   73), S(  38,   59),
    S(  26, -239), S( 103,  -30), S(  76,    4), S(  16,   16)
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
