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
    S(-40,   7), S(-21,   8), S(-37,   9), S(-24,  -4), S(-29,  18), S( 11,  18), S( 19,   9), S(-23, -22),
    S(-38,  -2), S(-40,   3), S(-21,  -4), S(-22,  -6), S(-12,   2), S(-24,   8), S(  3, -13), S(-20, -17),
    S(-32,  10), S(-32,   2), S(-18, -20), S( -3, -29), S(  3, -26), S( -2, -13), S(-14,  -9), S(-25, -16),
    S(-19,  32), S(-29,  10), S(-14,  -7), S(  0, -33), S(  5, -21), S( 25, -16), S(-11,  -1), S(-13,  11),
    S( 10,  49), S(-13,  28), S( 11,   2), S( 20, -21), S( 39,   0), S( 90,  16), S( 37,  31), S( 22,  41),
    S( 90,   4), S( 44,  -3), S( 55,  -7), S( 84, -30), S( 94, -11), S( 42,   4), S(-56,  22), S(-61,  29)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -41), S( -14,  -39), S( -12,  -19), S(  -7,   -2),
    S(  -8,  -24), S(  -3,   -7), S(   5,  -19), S(   4,   -7),
    S(   6,  -36), S(  10,   -7), S(  18,  -10), S(  15,   19),
    S(  17,    3), S(  13,   21), S(  36,   27), S(  23,   40),
    S(  50,   15), S(  38,   17), S(  56,   26), S(  46,   46),
    S( -18,   10), S(  42,    9), S(  48,   29), S(  64,   29),
    S(  -1,   -8), S( -38,   12), S(  41,    2), S(  44,   29),
    S(-174,  -81), S(-110,    7), S(-133,   11), S(  24,    7)
};

const Scorepair BishopSQT[32] = {
    S(  33,  -41), S(  28,  -15), S(  -3,  -10), S(   2,  -12),
    S(  37,  -40), S(  40,  -30), S(  35,  -13), S(  13,    1),
    S(  29,   -7), S(  35,   -1), S(  22,   -9), S(  17,   28),
    S(  17,  -25), S(  21,    7), S(  17,   23), S(  26,   33),
    S(   6,   -1), S(  22,   18), S(  26,   22), S(  27,   41),
    S(  41,    5), S(  16,   30), S(  17,    8), S(  36,   14),
    S( -61,    4), S( -67,   -5), S(  -6,   16), S( -18,   14),
    S( -54,  -19), S( -54,   14), S(-148,   15), S(-118,    9)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -36), S(  -8,  -26), S(  -8,  -15), S(  -3,  -24),
    S( -33,  -30), S( -20,  -32), S(  -8,  -17), S( -11,  -18),
    S( -30,  -23), S(  -7,  -18), S( -26,   -5), S( -19,   -6),
    S( -24,   -8), S( -23,    4), S( -25,   10), S(  -9,    0),
    S( -12,   12), S(   0,   20), S(  17,   14), S(  28,    8),
    S( -16,   25), S(  24,   17), S(  24,   20), S(  54,   11),
    S(  13,   26), S(  -6,   30), S(  39,   31), S(  47,   33),
    S(  23,   30), S(  24,   34), S(  15,   37), S(  21,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -75), S(   1,  -79), S(  19, -100), S(  28,  -82),
    S(  21,  -68), S(  23,  -70), S(  37,  -66), S(  22,  -21),
    S(  17,  -47), S(  27,  -24), S(  16,   20), S(  13,   12),
    S(  17,   -7), S(  22,   20), S(   3,   39), S(  -5,   65),
    S(  24,    7), S(  -3,   52), S(  10,   46), S( -12,   79),
    S(   9,   10), S(  -5,   44), S( -13,   71), S(  -2,   70),
    S( -13,   12), S( -46,   28), S( -20,   72), S( -36,   92),
    S( -46,   20), S( -18,   36), S( -15,   57), S( -13,   66)
};

const Scorepair KingSQT[32] = {
    S(  59, -118), S(  58,  -57), S( -31,  -42), S( -37,  -57),
    S(  53,  -51), S(  14,  -18), S( -19,   -5), S( -58,    1),
    S( -71,  -41), S(   6,  -15), S( -26,   10), S( -28,   21),
    S(-135,  -30), S( -42,    3), S( -27,   27), S( -22,   39),
    S( -79,    1), S(   9,   46), S(  12,   57), S(  -6,   59),
    S( -31,   24), S(  55,   75), S(  48,   83), S(  40,   68),
    S( -42,  -11), S(  15,   65), S(  43,   72), S(  38,   57),
    S(  26, -239), S( 104,  -29), S(  76,    4), S(  16,   16)
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
