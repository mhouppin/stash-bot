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
    S(-36,  10), S(-17,   5), S(-36,   8), S(-30,  -6), S(-20,   9), S(  7,  12), S( 15,   4), S(-25, -20),
    S(-35,  -5), S(-43,  -4), S(-28,  -5), S(-22,  -9), S(-11,  -4), S(-21,  -2), S(  3, -16), S(-22, -19),
    S(-29,  11), S(-34,   4), S(-15, -18), S( -7, -23), S( -1, -27), S(  0, -10), S(-10,  -9), S(-18, -13),
    S(-15,  30), S(-15,  11), S( -9,  -5), S( -4, -23), S( 10, -17), S( 26, -12), S( -4,   4), S(-17,  14),
    S( -4,  39), S(-13,  31), S( -2,   2), S( 24,  -5), S( 38,   7), S( 83,  24), S( 38,  26), S(  9,  37),
    S(108,   0), S( 25, -13), S( 48, -11), S( 85, -14), S( 80,  -6), S( 47,  15), S(-22,  14), S(-36,  16)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -60,  -43), S( -11,  -39), S( -19,  -17), S( -10,    5),
    S(  -7,  -18), S(   1,   -1), S(  13,   -9), S(   5,   -1),
    S(  11,  -29), S(  13,   -5), S(  23,  -11), S(  21,   16),
    S(  19,   -4), S(  17,   14), S(  45,   26), S(  37,   29),
    S(  47,   16), S(  49,   11), S(  53,   25), S(  52,   39),
    S(  -3,    1), S(  25,   14), S(  56,   30), S(  63,   24),
    S(  -2,    0), S( -42,   19), S(  30,    9), S(  37,   36),
    S(-161, -107), S(-119,   14), S(-185,   26), S(   2,   22)
};

const Scorepair BishopSQT[32] = {
    S(  17,  -45), S(  26,  -17), S(   4,   -3), S(   5,   -8),
    S(  38,  -39), S(  38,  -31), S(  32,  -12), S(  13,   -2),
    S(  26,  -12), S(  38,    0), S(  24,   -5), S(  21,   24),
    S(   9,  -26), S(  20,    9), S(  23,   18), S(  28,   23),
    S(  12,    1), S(  13,   18), S(  31,   24), S(  27,   34),
    S(  45,    4), S(  16,   22), S(  26,   21), S(  35,    7),
    S( -40,   16), S( -51,    3), S(  14,   19), S(  -4,    9),
    S( -40,   -9), S( -65,   22), S(-162,   24), S(-134,   24)
};

const Scorepair RookSQT[32] = {
    S( -10,  -34), S(   0,  -19), S( -10,  -23), S(  -2,  -19),
    S( -25,  -25), S( -15,  -24), S( -11,  -17), S(  -7,  -15),
    S( -19,  -18), S(   3,  -12), S( -21,   -1), S( -16,    1),
    S( -24,    1), S( -11,    8), S( -21,   11), S(  -3,    5),
    S(  -2,   18), S(  24,   19), S(  20,   12), S(  30,   10),
    S( -10,   23), S(  26,   13), S(  36,   17), S(  57,    8),
    S(   2,   28), S( -10,   25), S(  30,   20), S(  38,   24),
    S(   9,   38), S(  -2,   40), S(   4,   42), S(  10,   39)
};

const Scorepair QueenSQT[32] = {
    S(   7,  -71), S(   9,  -69), S(  17,  -90), S(  22,  -64),
    S(  15,  -61), S(  15,  -60), S(  36,  -54), S(  19,   -3),
    S(   9,  -44), S(  24,   -5), S(  20,   27), S(  11,   12),
    S(  13,   -4), S(  25,   19), S(   1,   34), S(  -7,   68),
    S(  29,   21), S(  -5,   52), S(   3,   25), S( -13,   60),
    S(   7,    1), S(   0,   41), S( -15,   51), S(   0,   50),
    S(  -1,   12), S( -48,    6), S( -31,   56), S( -35,   86),
    S( -52,   29), S(   1,   34), S( -25,   38), S( -13,   40)
};

const Scorepair KingSQT[32] = {
    S(  61, -117), S(  61,  -61), S( -16,  -39), S( -16,  -54),
    S(  64,  -56), S(  18,  -19), S( -11,  -14), S( -38,   -6),
    S( -37,  -37), S(   3,  -23), S( -21,    9), S( -10,   19),
    S(-152,  -14), S( -73,   12), S( -44,   31), S(  -7,   35),
    S(-113,   14), S( -19,   59), S(  24,   57), S(  41,   61),
    S( -51,   34), S(  54,   81), S(  60,   84), S(  75,   71),
    S( -68,  -20), S(  -5,   67), S(  31,   82), S(  54,   59),
    S(  27, -227), S(  69,  -32), S(  66,   22), S(  -5,   27)
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
