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
    S(-42,   1), S(-10,  13), S(-41,   9), S(-26,  -3), S(-31,  17), S(  7,  15), S( 23,  16), S(-25, -28),
    S(-40,  -9), S(-29,   9), S(-25,  -3), S(-24,  -6), S(-14,   1), S(-26,   5), S(  7,  -5), S(-21, -23),
    S(-34,   5), S(-21,   9), S(-21, -18), S( -5, -28), S(  1, -26), S( -4, -15), S(-10,  -1), S(-27, -21),
    S(-20,  25), S(-19,  17), S(-18,  -5), S( -2, -31), S(  5, -22), S( 23, -18), S( -6,   5), S(-15,   5),
    S(  7,  41), S( -8,  32), S(  9,   5), S( 19, -18), S( 39,   2), S( 87,  16), S( 36,  34), S( 21,  35),
    S( 89,  -4), S( 42,  -2), S( 54,  -6), S( 83, -27), S( 93,  -7), S( 41,   7), S(-51,  22), S(-59,  25)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -50,  -40), S( -14,  -40), S( -12,  -18), S(  -7,   -2),
    S(  -8,  -23), S(  -3,   -7), S(   5,  -18), S(   4,   -6),
    S(   6,  -35), S(  10,   -7), S(  18,  -10), S(  15,   18),
    S(  17,    3), S(  13,   21), S(  36,   26), S(  23,   39),
    S(  50,   14), S(  38,   17), S(  57,   26), S(  46,   45),
    S( -15,    9), S(  43,    8), S(  50,   28), S(  65,   28),
    S(  -1,   -7), S( -39,   14), S(  40,    2), S(  43,   30),
    S(-173,  -85), S(-110,    7), S(-140,   11), S(  23,    8)
};

const Scorepair BishopSQT[32] = {
    S(  33,  -39), S(  27,  -14), S(  -3,  -11), S(   2,  -12),
    S(  37,  -40), S(  40,  -29), S(  35,  -14), S(  13,    1),
    S(  29,   -7), S(  35,   -1), S(  22,   -9), S(  18,   27),
    S(  17,  -25), S(  20,    6), S(  18,   22), S(  26,   32),
    S(   6,   -1), S(  22,   16), S(  27,   21), S(  28,   40),
    S(  41,    5), S(  17,   29), S(  17,    9), S(  37,   13),
    S( -61,    5), S( -65,   -4), S(  -5,   16), S( -19,   14),
    S( -52,  -18), S( -57,   15), S(-150,   16), S(-122,    9)
};

const Scorepair RookSQT[32] = {
    S(  -7,  -36), S(  -8,  -26), S(  -8,  -15), S(  -3,  -23),
    S( -33,  -30), S( -21,  -32), S(  -8,  -17), S( -11,  -18),
    S( -29,  -23), S(  -7,  -18), S( -26,   -4), S( -19,   -6),
    S( -23,   -8), S( -24,    4), S( -25,   10), S( -10,    1),
    S( -11,   11), S(   0,   20), S(  17,   14), S(  28,    8),
    S( -16,   25), S(  23,   17), S(  24,   20), S(  54,   11),
    S(  12,   26), S(  -5,   30), S(  39,   30), S(  46,   33),
    S(  23,   29), S(  22,   34), S(  15,   37), S(  21,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -75), S(   0,  -77), S(  18,  -98), S(  27,  -80),
    S(  21,  -68), S(  22,  -69), S(  36,  -65), S(  22,  -19),
    S(  17,  -48), S(  26,  -23), S(  15,   22), S(  12,   13),
    S(  17,   -8), S(  21,   22), S(   3,   40), S(  -5,   66),
    S(  24,    7), S(  -4,   52), S(  10,   45), S( -12,   79),
    S(   9,    8), S(  -5,   43), S( -12,   70), S(  -1,   68),
    S( -14,    9), S( -43,   24), S( -21,   72), S( -37,   93),
    S( -48,   20), S( -18,   34), S( -15,   56), S( -12,   65)
};

const Scorepair KingSQT[32] = {
    S(  61, -118), S(  59,  -56), S( -29,  -41), S( -34,  -56),
    S(  55,  -51), S(  16,  -18), S( -17,   -5), S( -56,    1),
    S( -70,  -40), S(   8,  -14), S( -27,   11), S( -29,   22),
    S(-139,  -30), S( -46,    3), S( -30,   28), S( -22,   40),
    S( -82,    1), S(   7,   45), S(  13,   58), S(  -4,   60),
    S( -32,   23), S(  55,   73), S(  49,   83), S(  41,   69),
    S( -43,  -13), S(  14,   63), S(  43,   72), S(  38,   57),
    S(  26, -240), S( 103,  -31), S(  76,    3), S(  16,   16)
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
