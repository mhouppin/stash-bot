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
    S(-39,   7), S(-18,   7), S(-35,   8), S(-23,  -5), S(-27,  16), S( 13,  18), S( 22,   8), S(-23, -22),
    S(-36,  -2), S(-38,   2), S(-20,  -4), S(-21,  -7), S(-10,   2), S(-22,   8), S(  5, -13), S(-20, -17),
    S(-31,  10), S(-32,   3), S(-16, -20), S( -3, -29), S(  0, -25), S(  1, -14), S(-16,  -8), S(-24, -16),
    S(-17,  32), S(-25,  10), S(-14,  -7), S( -3, -30), S(  3, -19), S( 26, -16), S( -9,  -1), S(-14,  11),
    S(  9,  51), S(-12,  29), S( 10,   3), S( 18, -23), S( 36,   1), S( 86,  18), S( 32,  33), S( 21,  42),
    S( 94,  12), S( 37,   0), S( 51, -10), S( 78, -38), S( 82, -20), S( 34,   4), S(-44,  18), S(-55,  31)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -38), S( -16,  -37), S( -15,  -14), S( -10,    3),
    S( -11,  -21), S(  -5,   -3), S(   5,  -16), S(   4,   -2),
    S(   7,  -32), S(  13,   -7), S(  23,   -7), S(  19,   20),
    S(  17,    2), S(  18,   19), S(  44,   23), S(  30,   39),
    S(  47,   13), S(  47,   12), S(  60,   21), S(  59,   41),
    S(  -7,    6), S(  28,    8), S(  58,   24), S(  59,   24),
    S(  -8,   -6), S( -42,   16), S(  34,    4), S(  36,   31),
    S(-166,  -91), S(-112,    8), S(-155,   13), S(  19,    9)
};

const Scorepair BishopSQT[32] = {
    S(  31,  -35), S(  27,  -11), S(  -4,   -6), S(   2,   -9),
    S(  38,  -40), S(  40,  -26), S(  36,  -13), S(  15,    3),
    S(  27,   -9), S(  38,   -2), S(  24,   -6), S(  20,   27),
    S(  11,  -24), S(  21,    5), S(  19,   19), S(  24,   27),
    S(   9,   -2), S(  14,   15), S(  27,   18), S(  26,   33),
    S(  43,    5), S(  19,   28), S(  17,   10), S(  36,   10),
    S( -57,    6), S( -57,   -2), S(  -2,   15), S( -15,   13),
    S( -48,  -14), S( -61,   17), S(-152,   17), S(-131,   10)
};

const Scorepair RookSQT[32] = {
    S(  -7,  -28), S(  -7,  -20), S(  -7,   -9), S(  -2,  -19),
    S( -32,  -23), S( -19,  -25), S(  -7,  -11), S( -10,  -12),
    S( -28,  -18), S(  -5,  -15), S( -25,    0), S( -17,   -1),
    S( -22,   -6), S( -20,    4), S( -25,    9), S(  -9,    1),
    S(  -6,    9), S(   4,   15), S(  17,    8), S(  29,    2),
    S( -13,   20), S(  24,   10), S(  25,   11), S(  53,    0),
    S(  12,   22), S(  -5,   24), S(  35,   24), S(  46,   25),
    S(  15,   31), S(  14,   35), S(  10,   37), S(  14,   33)
};

const Scorepair QueenSQT[32] = {
    S(  17,  -72), S(   1,  -70), S(  19,  -90), S(  27,  -67),
    S(  23,  -66), S(  23,  -61), S(  36,  -57), S(  23,  -11),
    S(  18,  -45), S(  26,  -19), S(  15,   26), S(  13,   17),
    S(  17,   -7), S(  20,   24), S(   2,   38), S(  -7,   65),
    S(  23,    6), S(  -4,   49), S(   8,   39), S( -15,   72),
    S(  11,    4), S(  -8,   38), S( -15,   62), S(  -4,   59),
    S( -10,    3), S( -42,   13), S( -26,   67), S( -43,   88),
    S( -52,   22), S( -16,   30), S( -17,   55), S( -11,   60)
};

const Scorepair KingSQT[32] = {
    S(  64, -123), S(  62,  -60), S( -25,  -45), S( -32,  -60),
    S(  58,  -56), S(  18,  -22), S( -15,   -8), S( -54,   -2),
    S( -68,  -41), S(  13,  -16), S( -23,   10), S( -28,   21),
    S(-151,  -29), S( -57,    5), S( -37,   29), S( -21,   40),
    S( -89,    5), S(   2,   51), S(  16,   64), S(   3,   66),
    S( -35,   25), S(  56,   79), S(  51,   89), S(  46,   76),
    S( -45,  -15), S(  12,   61), S(  42,   74), S(  39,   59),
    S(  26, -241), S( 102,  -35), S(  74,    1), S(  14,   14)
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
