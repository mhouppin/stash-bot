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
    S(-40,   7), S(-21,   8), S(-37,   8), S(-24,  -4), S(-29,  18), S( 11,  18), S( 20,   9), S(-23, -22),
    S(-38,  -2), S(-40,   3), S(-21,  -4), S(-22,  -6), S(-12,   2), S(-24,   8), S(  3, -13), S(-19, -17),
    S(-32,  10), S(-32,   2), S(-18, -20), S( -3, -29), S(  3, -26), S( -2, -13), S(-14,  -9), S(-25, -16),
    S(-19,  32), S(-29,  10), S(-14,  -7), S(  0, -33), S(  5, -21), S( 24, -16), S(-10,  -1), S(-13,  11),
    S( 10,  49), S(-13,  28), S( 11,   2), S( 20, -21), S( 39,   1), S( 89,  17), S( 36,  32), S( 22,  41),
    S( 91,   3), S( 42,  -4), S( 54,  -8), S( 83, -29), S( 93,  -9), S( 42,   5), S(-53,  22), S(-60,  29)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -40), S( -14,  -39), S( -12,  -19), S(  -7,   -2),
    S(  -8,  -23), S(  -3,   -7), S(   5,  -19), S(   4,   -6),
    S(   6,  -35), S(  10,   -7), S(  18,  -10), S(  15,   18),
    S(  17,    3), S(  13,   21), S(  35,   26), S(  23,   40),
    S(  50,   14), S(  38,   17), S(  57,   26), S(  46,   45),
    S( -16,    9), S(  43,    8), S(  49,   29), S(  65,   29),
    S(  -1,   -7), S( -39,   13), S(  41,    2), S(  43,   30),
    S(-173,  -83), S(-110,    7), S(-137,   11), S(  23,    8)
};

const Scorepair BishopSQT[32] = {
    S(  33,  -40), S(  28,  -14), S(  -3,  -10), S(   2,  -12),
    S(  37,  -40), S(  40,  -29), S(  35,  -13), S(  13,    1),
    S(  29,   -7), S(  35,   -1), S(  22,   -9), S(  18,   27),
    S(  17,  -25), S(  21,    7), S(  18,   22), S(  26,   32),
    S(   6,   -1), S(  22,   17), S(  27,   22), S(  28,   40),
    S(  41,    5), S(  16,   30), S(  17,    8), S(  36,   13),
    S( -61,    5), S( -67,   -5), S(  -5,   16), S( -19,   14),
    S( -53,  -19), S( -56,   15), S(-149,   16), S(-121,    9)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -36), S(  -8,  -26), S(  -8,  -15), S(  -3,  -24),
    S( -33,  -30), S( -20,  -32), S(  -8,  -17), S( -11,  -18),
    S( -30,  -23), S(  -7,  -18), S( -26,   -5), S( -20,   -6),
    S( -24,   -8), S( -23,    4), S( -25,    9), S( -10,    0),
    S( -12,   12), S(   0,   20), S(  17,   14), S(  28,    8),
    S( -16,   25), S(  24,   17), S(  24,   20), S(  54,   11),
    S(  13,   26), S(  -6,   30), S(  39,   31), S(  47,   33),
    S(  23,   30), S(  23,   34), S(  15,   37), S(  21,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -75), S(   1,  -78), S(  18,  -99), S(  27,  -81),
    S(  21,  -68), S(  22,  -69), S(  36,  -65), S(  22,  -20),
    S(  17,  -48), S(  27,  -23), S(  15,   21), S(  13,   13),
    S(  17,   -8), S(  21,   21), S(   3,   40), S(  -5,   66),
    S(  24,    7), S(  -3,   52), S(  10,   45), S( -12,   79),
    S(  10,    9), S(  -5,   44), S( -13,   71), S(  -2,   69),
    S( -13,   10), S( -45,   25), S( -21,   72), S( -37,   93),
    S( -48,   20), S( -18,   35), S( -15,   57), S( -12,   65)
};

const Scorepair KingSQT[32] = {
    S(  60, -119), S(  59,  -56), S( -30,  -41), S( -35,  -56),
    S(  55,  -51), S(  15,  -18), S( -18,   -5), S( -57,    1),
    S( -70,  -40), S(   7,  -14), S( -26,   10), S( -29,   21),
    S(-138,  -29), S( -45,    4), S( -29,   28), S( -22,   39),
    S( -81,    2), S(   8,   46), S(  12,   57), S(  -5,   59),
    S( -32,   24), S(  55,   75), S(  48,   83), S(  41,   68),
    S( -42,  -12), S(  14,   64), S(  43,   72), S(  38,   57),
    S(  26, -239), S( 104,  -30), S(  76,    3), S(  16,   16)
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
