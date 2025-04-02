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
    S(-40,   7), S(-19,   7), S(-36,   7), S(-24,  -5), S(-27,  16), S( 13,  17), S( 21,   8), S(-23, -22),
    S(-38,  -2), S(-39,   2), S(-20,  -5), S(-21,  -7), S(-11,   1), S(-22,   8), S(  4, -14), S(-20, -17),
    S(-32,  10), S(-33,   3), S(-17, -21), S( -3, -29), S( -1, -26), S(  0, -14), S(-16,  -9), S(-24, -16),
    S(-19,  32), S(-27,  10), S(-14,  -7), S( -3, -31), S(  2, -19), S( 25, -16), S(-10,  -1), S(-15,  11),
    S(  9,  49), S(-12,  28), S( 10,   3), S( 20, -20), S( 38,   2), S( 87,  18), S( 34,  33), S( 21,  41),
    S( 92,   1), S( 39,  -5), S( 53,  -8), S( 81, -28), S( 92,  -7), S( 41,   8), S(-48,  21), S(-57,  28)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -39), S( -16,  -38), S( -15,  -15), S( -10,    2),
    S( -11,  -22), S(  -5,   -4), S(   4,  -16), S(   4,   -2),
    S(   6,  -33), S(  12,   -7), S(  23,   -6), S(  18,   20),
    S(  16,    2), S(  17,   20), S(  44,   23), S(  30,   40),
    S(  46,   13), S(  46,   12), S(  59,   22), S(  58,   42),
    S( -11,    7), S(  29,    8), S(  56,   25), S(  59,   26),
    S(  -6,   -7), S( -41,   15), S(  36,    3), S(  39,   30),
    S(-170,  -87), S(-111,    8), S(-145,   12), S(  21,    8)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -37), S(  27,  -12), S(  -4,   -6), S(   2,   -9),
    S(  38,  -40), S(  40,  -26), S(  36,  -13), S(  15,    3),
    S(  27,   -9), S(  37,   -2), S(  24,   -7), S(  20,   28),
    S(  11,  -24), S(  21,    5), S(  18,   20), S(  24,   29),
    S(   9,   -2), S(  13,   16), S(  26,   19), S(  26,   35),
    S(  43,    5), S(  18,   29), S(  17,    9), S(  36,   11),
    S( -58,    6), S( -58,   -3), S(  -3,   16), S( -16,   14),
    S( -50,  -16), S( -58,   16), S(-150,   17), S(-125,   10)
};

const Scorepair RookSQT[32] = {
    S(  -7,  -29), S(  -7,  -20), S(  -7,  -10), S(  -2,  -19),
    S( -32,  -24), S( -19,  -25), S(  -7,  -11), S( -10,  -12),
    S( -28,  -18), S(  -5,  -15), S( -25,   -1), S( -17,   -1),
    S( -23,   -6), S( -21,    4), S( -25,    9), S(  -9,    1),
    S(  -7,    9), S(   3,   16), S(  17,    9), S(  28,    3),
    S( -14,   21), S(  23,   10), S(  24,   12), S(  52,    1),
    S(  11,   22), S(  -6,   24), S(  35,   24), S(  45,   26),
    S(  18,   30), S(  18,   34), S(  12,   37), S(  17,   32)
};

const Scorepair QueenSQT[32] = {
    S(  18,  -73), S(   2,  -74), S(  21,  -94), S(  28,  -71),
    S(  23,  -67), S(  24,  -65), S(  37,  -60), S(  24,  -14),
    S(  18,  -46), S(  27,  -21), S(  16,   24), S(  13,   15),
    S(  17,   -8), S(  21,   23), S(   2,   38), S(  -7,   65),
    S(  24,    6), S(  -4,   50), S(   8,   42), S( -16,   75),
    S(  11,    6), S(  -8,   40), S( -16,   66), S(  -5,   63),
    S( -11,    7), S( -44,   18), S( -25,   69), S( -41,   90),
    S( -50,   21), S( -17,   33), S( -16,   56), S( -12,   63)
};

const Scorepair KingSQT[32] = {
    S(  62, -119), S(  60,  -57), S( -26,  -41), S( -33,  -56),
    S(  55,  -52), S(  17,  -18), S( -16,   -5), S( -55,    1),
    S( -70,  -39), S(  10,  -14), S( -25,   11), S( -29,   22),
    S(-143,  -28), S( -50,    5), S( -33,   28), S( -22,   40),
    S( -84,    3), S(   5,   47), S(  14,   58), S(  -2,   59),
    S( -33,   24), S(  55,   75), S(  49,   83), S(  43,   67),
    S( -44,  -13), S(  13,   62), S(  42,   72), S(  38,   56),
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
