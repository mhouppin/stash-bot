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
    S(-40,   6), S(-18,   5), S(-36,   6), S(-24,  -6), S(-29,  14), S( 11,  16), S( 18,   5), S(-25, -22),
    S(-38,  -4), S(-39,  -2), S(-22,  -7), S(-23, -10), S(-13,  -2), S(-25,   4), S(  1, -18), S(-22, -18),
    S(-31,  11), S(-31,   4), S(-16, -17), S( -2, -23), S( -1, -23), S(  0, -12), S(-17,  -7), S(-25, -14),
    S(-17,  32), S(-24,  10), S(-12,  -4), S( -1, -26), S(  3, -16), S( 24, -13), S( -9,   1), S(-15,  12),
    S(  8,  47), S(-12,  26), S(  9,   2), S( 22, -12), S( 37,   5), S( 78,  20), S( 32,  34), S( 19,  40),
    S( 97,  -4), S( 32,  -8), S( 52,  -8), S( 80, -24), S( 82,  -4), S( 36,  11), S(-25,  15), S(-40,  19)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -49,  -42), S( -14,  -39), S( -12,  -17), S(  -7,    0),
    S(  -8,  -24), S(  -2,   -6), S(   7,  -17), S(   6,   -4),
    S(   9,  -34), S(  14,   -8), S(  25,   -7), S(  20,   18),
    S(  18,    0), S(  19,   17), S(  45,   21), S(  31,   35),
    S(  47,   16), S(  47,   13), S(  61,   22), S(  57,   40),
    S(  -5,    8), S(  29,   11), S(  59,   27), S(  55,   26),
    S(  -8,   -2), S( -40,   17), S(  33,    7), S(  33,   32),
    S(-155, -101), S(-115,   11), S(-181,   21), S(  10,   10)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -38), S(  28,  -17), S(  -2,   -7), S(   4,  -10),
    S(  39,  -43), S(  41,  -27), S(  37,  -15), S(  16,    1),
    S(  28,  -11), S(  39,   -4), S(  25,   -7), S(  22,   25),
    S(  12,  -24), S(  22,    4), S(  20,   17), S(  25,   25),
    S(   9,    1), S(  15,   16), S(  27,   19), S(  27,   33),
    S(  42,    7), S(  20,   29), S(  18,   13), S(  35,   11),
    S( -50,    7), S( -55,    0), S(  -2,   16), S( -15,   15),
    S( -46,  -12), S( -70,   21), S(-156,   19), S(-148,   14)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -33), S(  -6,  -23), S(  -6,  -14), S(   0,  -22),
    S( -30,  -29), S( -16,  -28), S(  -7,  -14), S(  -8,  -15),
    S( -27,  -22), S(  -3,  -17), S( -26,   -4), S( -16,   -4),
    S( -21,   -7), S( -18,    4), S( -26,    8), S(  -7,    0),
    S(  -5,   14), S(   5,   18), S(  15,   11), S(  29,    5),
    S( -13,   24), S(  25,   12), S(  23,   13), S(  52,    4),
    S(  11,   24), S(  -5,   24), S(  31,   24), S(  45,   26),
    S(   8,   33), S(   2,   37), S(   3,   37), S(   6,   34)
};

const Scorepair QueenSQT[32] = {
    S(  14,  -68), S(  -1,  -62), S(  17,  -86), S(  25,  -61),
    S(  21,  -64), S(  21,  -58), S(  34,  -56), S(  21,   -7),
    S(  16,  -42), S(  24,  -16), S(  13,   24), S(  11,   19),
    S(  15,   -4), S(  19,   27), S(   1,   34), S(  -9,   65),
    S(  21,   13), S(  -5,   50), S(   7,   34), S( -14,   68),
    S(  10,    5), S(  -7,   36), S( -13,   54), S(  -3,   49),
    S(  -9,    2), S( -39,    7), S( -31,   58), S( -45,   83),
    S( -54,   27), S( -11,   25), S( -20,   48), S( -10,   53)
};

const Scorepair KingSQT[32] = {
    S(  67, -118), S(  66,  -60), S( -20,  -44), S( -26,  -59),
    S(  66,  -52), S(  21,  -22), S( -10,   -7), S( -48,   -2),
    S( -44,  -38), S(  14,  -16), S( -22,   10), S( -25,   21),
    S(-165,  -20), S( -74,    7), S( -47,   30), S( -20,   40),
    S(-103,   10), S( -12,   49), S(  21,   60), S(  17,   60),
    S( -42,   29), S(  56,   74), S(  55,   83), S(  56,   68),
    S( -50,  -11), S(   7,   59), S(  39,   71), S(  38,   54),
    S(  24, -240), S(  97,  -38), S(  71,    1), S(  11,   12)
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
