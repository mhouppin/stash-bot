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
    S(-34,  11), S(-18,   6), S(-39,  10), S(-33,  -7), S(-20,   5), S(  8,  13), S( 17,   5), S(-24, -21),
    S(-34,  -1), S(-45,  -2), S(-26,  -5), S(-23,  -8), S(-11,  -6), S(-24,   1), S(  3, -15), S(-21, -19),
    S(-28,  11), S(-35,   6), S(-14, -15), S( -7, -23), S(  0, -28), S(  1,  -9), S(-13,  -8), S(-21, -14),
    S(-14,  34), S(-17,  13), S( -8,  -5), S( -1, -25), S( 13, -19), S( 24,  -7), S( -4,   6), S(-13,  17),
    S( -1,  44), S(-13,  33), S( -4,   5), S( 24,  -3), S( 45,   8), S( 86,  30), S( 35,  27), S( 12,  43),
    S(115,   8), S( 30, -11), S( 52, -10), S( 90, -10), S( 78,  -6), S( 50,  23), S(-29,  18), S(-35,  16)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -70,  -36), S( -10,  -45), S( -24,  -11), S( -14,    9),
    S(  -5,  -18), S(  -4,    3), S(   7,   -9), S(   6,   -1),
    S(   8,  -22), S(  14,   -1), S(  24,  -11), S(  23,   19),
    S(  17,   -1), S(  19,   17), S(  47,   27), S(  36,   32),
    S(  47,   22), S(  55,   16), S(  60,   30), S(  51,   43),
    S(   1,    9), S(  31,   18), S(  67,   31), S(  58,   29),
    S(  -1,    2), S( -34,   21), S(  39,   16), S(  46,   39),
    S(-167, -113), S(-109,   21), S(-157,   37), S(   2,   29)
};

const Scorepair BishopSQT[32] = {
    S(  13,  -42), S(  25,  -14), S(   3,   -3), S(   5,   -7),
    S(  33,  -32), S(  38,  -30), S(  35,  -10), S(  14,    1),
    S(  26,   -4), S(  38,    8), S(  23,   -3), S(  23,   25),
    S(   6,  -31), S(  25,   10), S(  25,   21), S(  30,   30),
    S(  13,    6), S(  16,   18), S(  40,   27), S(  32,   41),
    S(  43,    8), S(  24,   22), S(  31,   16), S(  45,   18),
    S( -30,   14), S( -52,    7), S(   6,   20), S(   9,   10),
    S( -23,    7), S( -69,   22), S(-161,   27), S(-134,   18)
};

const Scorepair RookSQT[32] = {
    S(  -8,  -26), S(   0,  -23), S(  -8,  -19), S(  -3,  -16),
    S( -31,  -27), S( -20,  -21), S( -10,  -16), S(  -7,  -10),
    S( -24,  -17), S(   7,   -8), S( -19,   -1), S( -16,    3),
    S( -19,   -1), S(  -6,   13), S( -24,   15), S(  -6,   10),
    S(   1,   27), S(  22,   29), S(  23,   15), S(  28,   16),
    S(  -6,   36), S(  33,   17), S(  39,   20), S(  58,   12),
    S(  11,   38), S(  -1,   31), S(  30,   28), S(  47,   34),
    S(   3,   42), S(   1,   45), S(   5,   53), S(   6,   39)
};

const Scorepair QueenSQT[32] = {
    S(   7,  -76), S(   4,  -73), S(  16,  -92), S(  24,  -61),
    S(  14,  -70), S(  12,  -53), S(  33,  -51), S(  20,    0),
    S(  13,  -34), S(  26,   -3), S(  17,   24), S(  11,   24),
    S(  14,    0), S(  26,   23), S(   3,   46), S(  -4,   65),
    S(  30,   22), S(  -5,   59), S(   1,   36), S( -14,   67),
    S(   1,    8), S(   3,   42), S( -14,   58), S(  -2,   55),
    S(  -5,    7), S( -49,   14), S( -32,   57), S( -25,   88),
    S( -43,   30), S(   3,   29), S( -18,   48), S(  -8,   51)
};

const Scorepair KingSQT[32] = {
    S(  57, -121), S(  64,  -67), S( -14,  -47), S( -17,  -61),
    S(  60,  -63), S(  16,  -23), S(  -7,  -14), S( -36,   -7),
    S( -38,  -43), S(  -1,  -20), S( -21,    7), S(  -7,   21),
    S(-158,  -19), S( -80,   13), S( -38,   32), S(  14,   43),
    S(-106,   26), S( -22,   65), S(  28,   65), S(  50,   71),
    S( -53,   52), S(  54,   92), S(  76,  101), S(  82,   85),
    S( -64,   -8), S(   5,   82), S(  25,   97), S(  59,   74),
    S(  27, -208), S(  77,  -16), S(  55,   35), S( -23,   26)
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
