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
    S(-39,   6), S(-20,   7), S(-37,   7), S(-24,  -5), S(-29,  17), S( 10,  16), S( 19,   7), S(-24, -23),
    S(-37,  -4), S(-41,  -1), S(-22,  -7), S(-23,  -9), S(-14,  -1), S(-26,   5), S(  1, -17), S(-21, -19),
    S(-31,  12), S(-31,   5), S(-17, -17), S( -1, -24), S(  3, -24), S( -2, -12), S(-14,  -7), S(-25, -14),
    S(-17,  33), S(-27,  12), S(-12,  -5), S(  1, -31), S(  6, -18), S( 24, -14), S( -9,   1), S(-13,  13),
    S( 11,  49), S(-12,  27), S( 11,   2), S( 20, -20), S( 38,   1), S( 86,  17), S( 34,  32), S( 23,  41),
    S( 92,   0), S( 39,  -6), S( 53,  -9), S( 81, -29), S( 92,  -7), S( 41,   8), S(-48,  21), S(-56,  28)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -50,  -39), S( -14,  -40), S( -13,  -18), S(  -7,   -2),
    S(  -8,  -22), S(  -4,   -6), S(   6,  -19), S(   4,   -6),
    S(   6,  -35), S(  10,   -7), S(  19,  -10), S(  15,   18),
    S(  17,    3), S(  13,   21), S(  36,   26), S(  23,   39),
    S(  51,   14), S(  39,   17), S(  57,   26), S(  47,   45),
    S( -13,    8), S(  44,    7), S(  50,   28), S(  65,   28),
    S(   0,   -6), S( -40,   14), S(  40,    3), S(  41,   31),
    S(-171,  -88), S(-111,    8), S(-146,   12), S(  21,    9)
};

const Scorepair BishopSQT[32] = {
    S(  33,  -38), S(  27,  -13), S(  -2,  -10), S(   3,  -12),
    S(  37,  -40), S(  40,  -29), S(  36,  -14), S(  13,    1),
    S(  29,   -7), S(  36,   -1), S(  22,   -9), S(  18,   27),
    S(  17,  -25), S(  21,    6), S(  18,   21), S(  26,   31),
    S(   6,   -1), S(  22,   17), S(  27,   21), S(  28,   39),
    S(  41,    5), S(  17,   29), S(  17,    9), S(  37,   12),
    S( -61,    5), S( -66,   -4), S(  -4,   16), S( -20,   15),
    S( -51,  -18), S( -60,   16), S(-152,   16), S(-126,   10)
};

const Scorepair RookSQT[32] = {
    S(  -7,  -36), S(  -8,  -26), S(  -8,  -15), S(  -3,  -24),
    S( -33,  -30), S( -20,  -31), S(  -8,  -17), S( -11,  -18),
    S( -30,  -23), S(  -7,  -19), S( -26,   -5), S( -20,   -6),
    S( -24,   -8), S( -22,    5), S( -25,   10), S( -10,    1),
    S( -10,   12), S(   1,   20), S(  17,   14), S(  28,    8),
    S( -15,   25), S(  24,   17), S(  24,   20), S(  54,   11),
    S(  13,   26), S(  -6,   29), S(  39,   30), S(  45,   33),
    S(  22,   30), S(  20,   34), S(  14,   36), S(  20,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -75), S(  -1,  -75), S(  18,  -97), S(  26,  -79),
    S(  21,  -68), S(  22,  -68), S(  36,  -64), S(  21,  -18),
    S(  16,  -48), S(  26,  -22), S(  14,   23), S(  12,   14),
    S(  17,   -9), S(  21,   23), S(   2,   40), S(  -6,   67),
    S(  24,    7), S(  -4,   52), S(  10,   43), S( -13,   78),
    S(  10,    7), S(  -5,   43), S( -13,   70), S(  -1,   67),
    S( -12,    7), S( -44,   21), S( -21,   72), S( -38,   93),
    S( -50,   21), S( -17,   33), S( -15,   56), S( -11,   64)
};

const Scorepair KingSQT[32] = {
    S(  63, -118), S(  61,  -56), S( -27,  -41), S( -33,  -56),
    S(  56,  -51), S(  17,  -18), S( -16,   -5), S( -55,    1),
    S( -69,  -40), S(   9,  -14), S( -26,   11), S( -30,   22),
    S(-144,  -28), S( -50,    5), S( -33,   29), S( -22,   40),
    S( -84,    3), S(   5,   47), S(  14,   58), S(  -2,   59),
    S( -33,   24), S(  55,   74), S(  49,   82), S(  43,   67),
    S( -44,  -13), S(  13,   62), S(  42,   71), S(  38,   56),
    S(  26, -240), S( 103,  -32), S(  75,    2), S(  15,   15)
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
