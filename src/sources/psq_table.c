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
    S(-39,   6), S(-18,   5), S(-36,   6), S(-24,  -7), S(-28,  14), S( 11,  15), S( 20,   6), S(-24, -23),
    S(-37,  -4), S(-39,  -2), S(-22,  -7), S(-23, -10), S(-13,  -2), S(-25,   4), S(  2, -17), S(-22, -19),
    S(-31,  11), S(-31,   5), S(-16, -18), S( -2, -24), S( -1, -23), S(  0, -13), S(-16,  -7), S(-24, -14),
    S(-17,  33), S(-25,  11), S(-12,  -4), S( -2, -28), S(  3, -16), S( 25, -14), S( -9,   1), S(-14,  13),
    S(  9,  49), S(-11,  28), S(  9,   2), S( 20, -17), S( 37,   3), S( 84,  18), S( 32,  33), S( 20,  41),
    S( 96,   0), S( 33,  -6), S( 51,  -7), S( 79, -24), S( 86,  -4), S( 38,  11), S(-33,  19), S(-47,  22)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -40), S( -15,  -38), S( -14,  -15), S(  -9,    2),
    S( -10,  -23), S(  -4,   -4), S(   5,  -16), S(   5,   -4),
    S(   7,  -34), S(  13,   -8), S(  24,   -8), S(  19,   18),
    S(  17,    1), S(  18,   18), S(  44,   22), S(  30,   37),
    S(  47,   16), S(  47,   13), S(  60,   22), S(  59,   42),
    S(  -5,    7), S(  28,   10), S(  58,   25), S(  59,   26),
    S(  -8,   -4), S( -42,   17), S(  33,    5), S(  34,   32),
    S(-161,  -96), S(-113,   10), S(-167,   16), S(  16,    9)
};

const Scorepair BishopSQT[32] = {
    S(  31,  -38), S(  27,  -14), S(  -3,   -7), S(   3,  -10),
    S(  38,  -43), S(  40,  -27), S(  36,  -14), S(  15,    1),
    S(  27,  -11), S(  38,   -3), S(  24,   -7), S(  21,   25),
    S(  12,  -24), S(  22,    4), S(  19,   18), S(  24,   26),
    S(   9,    1), S(  14,   16), S(  27,   19), S(  27,   34),
    S(  43,    7), S(  19,   29), S(  17,   11), S(  36,   12),
    S( -57,    7), S( -56,   -1), S(  -2,   15), S( -14,   14),
    S( -47,  -13), S( -65,   20), S(-154,   18), S(-138,   12)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -31), S(  -7,  -22), S(  -7,  -11), S(  -2,  -20),
    S( -32,  -26), S( -19,  -26), S(  -7,  -12), S( -10,  -13),
    S( -28,  -20), S(  -5,  -16), S( -25,   -2), S( -18,   -2),
    S( -22,   -7), S( -20,    4), S( -25,    9), S( -10,    1),
    S(  -5,   12), S(   5,   16), S(  18,   10), S(  29,    4),
    S( -13,   22), S(  24,   11), S(  26,   12), S(  52,    2),
    S(  11,   22), S(  -4,   23), S(  34,   23), S(  45,   25),
    S(  11,   31), S(   8,   35), S(   8,   36), S(  11,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -69), S(  -1,  -66), S(  18,  -86), S(  25,  -64),
    S(  22,  -65), S(  21,  -59), S(  35,  -55), S(  21,   -9),
    S(  16,  -44), S(  25,  -18), S(  14,   26), S(  11,   17),
    S(  16,   -6), S(  19,   25), S(   1,   37), S(  -8,   64),
    S(  22,    9), S(  -5,   49), S(   8,   38), S( -15,   70),
    S(  11,    4), S(  -7,   36), S( -14,   59), S(  -3,   55),
    S( -10,    1), S( -41,    7), S( -26,   64), S( -44,   86),
    S( -54,   24), S( -15,   26), S( -18,   52), S( -10,   56)
};

const Scorepair KingSQT[32] = {
    S(  66, -119), S(  65,  -57), S( -21,  -42), S( -27,  -56),
    S(  59,  -53), S(  21,  -19), S( -12,   -6), S( -50,    0),
    S( -65,  -40), S(  14,  -14), S( -21,   10), S( -25,   21),
    S(-160,  -26), S( -64,    7), S( -41,   29), S( -20,   40),
    S( -96,    6), S(  -4,   49), S(  18,   59), S(   8,   60),
    S( -38,   27), S(  56,   75), S(  52,   82), S(  50,   67),
    S( -47,  -13), S(  10,   60), S(  40,   71), S(  38,   56),
    S(  25, -241), S( 100,  -37), S(  73,    1), S(  13,   13)
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
