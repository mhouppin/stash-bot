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
    S(-39,   5), S(-20,   5), S(-38,   6), S(-24,  -7), S(-30,  14), S(  9,  14), S( 16,   4), S(-25, -23),
    S(-37,  -6), S(-40,  -3), S(-22,  -9), S(-24, -11), S(-14,  -3), S(-27,   3), S( -1, -19), S(-22, -20),
    S(-31,  10), S(-30,   2), S(-17, -18), S( -1, -25), S(  2, -25), S( -2, -13), S(-16,  -8), S(-26, -15),
    S(-16,  30), S(-26,   9), S(-11,  -6), S(  2, -30), S(  6, -18), S( 23, -15), S(-10,   0), S(-14,  10),
    S(  9,  46), S(-12,  25), S(  8,   1), S( 20, -12), S( 34,   5), S( 75,  19), S( 29,  34), S( 20,  39),
    S( 99,  -5), S( 31,  -9), S( 54,  -9), S( 85, -25), S( 76,  -1), S( 33,  13), S(-21,  11), S(-28,  18)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -39,  -46), S(  -8,  -41), S(  -6,  -21), S(  -1,   -3),
    S(  -2,  -26), S(   3,  -10), S(  11,  -20), S(   9,   -7),
    S(  10,  -36), S(  15,   -8), S(  24,  -11), S(  19,   15),
    S(  20,    1), S(  16,   17), S(  38,   23), S(  25,   33),
    S(  49,   16), S(  40,   16), S(  49,   26), S(  43,   42),
    S(  -5,    9), S(  29,   12), S(  49,   29), S(  42,   29),
    S(  -2,   -3), S( -34,   17), S(  32,    7), S(  31,   32),
    S(-144, -104), S(-118,   12), S(-184,   25), S(  -3,   13)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -41), S(  27,  -19), S(  -3,  -11), S(   3,  -12),
    S(  38,  -44), S(  40,  -29), S(  37,  -16), S(  14,    0),
    S(  28,  -10), S(  38,   -3), S(  24,   -8), S(  20,   25),
    S(  17,  -25), S(  24,    6), S(  20,   20), S(  29,   30),
    S(   8,    1), S(  24,   17), S(  29,   21), S(  33,   38),
    S(  43,    7), S(  21,   31), S(  23,   12), S(  38,   13),
    S( -49,    5), S( -54,   -2), S(  -1,   16), S( -14,   16),
    S( -44,  -16), S( -76,   20), S(-146,   18), S(-167,   16)
};

const Scorepair RookSQT[32] = {
    S(  -5,  -36), S(  -6,  -26), S(  -6,  -16), S(   0,  -24),
    S( -30,  -34), S( -16,  -32), S(  -7,  -19), S(  -8,  -20),
    S( -28,  -25), S(  -3,  -19), S( -26,   -6), S( -16,   -7),
    S( -22,   -8), S( -19,    4), S( -25,    8), S(  -7,    0),
    S(  -9,   16), S(   2,   22), S(  15,   15), S(  29,    9),
    S( -14,   28), S(  25,   18), S(  21,   20), S(  53,   11),
    S(  13,   27), S(  -6,   29), S(  33,   29), S(  42,   32),
    S(  19,   33), S(   7,   38), S(   3,   38), S(  13,   34)
};

const Scorepair QueenSQT[32] = {
    S(  15,  -74), S(  -2,  -66), S(  17,  -94), S(  25,  -70),
    S(  20,  -68), S(  20,  -63), S(  35,  -61), S(  20,  -12),
    S(  16,  -46), S(  24,  -18), S(  13,   23), S(  10,   20),
    S(  15,   -5), S(  18,   28), S(   1,   39), S(  -9,   75),
    S(  21,   13), S(  -6,   57), S(   9,   38), S( -15,   79),
    S(  11,    6), S(  -6,   47), S( -13,   64), S(   0,   54),
    S( -10,    4), S( -45,   19), S( -29,   67), S( -44,   90),
    S( -55,   26), S(  -3,   15), S( -18,   42), S(  -5,   47)
};

const Scorepair KingSQT[32] = {
    S(  66, -115), S(  66,  -60), S( -20,  -43), S( -25,  -59),
    S(  66,  -53), S(  22,  -22), S( -10,   -8), S( -48,   -2),
    S( -28,  -43), S(  12,  -16), S( -24,   10), S( -27,   21),
    S(-157,  -21), S( -97,    9), S( -66,   32), S( -29,   41),
    S(-132,   12), S( -61,   51), S(  26,   60), S(  49,   59),
    S( -81,   32), S(  56,   73), S(  85,   82), S( 113,   66),
    S( -96,   -5), S( -32,   61), S(  22,   72), S(  38,   54),
    S(  -1, -232), S(  11,  -35), S(   7,    3), S( -49,   13)
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
