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
    S(-39,   7), S(-18,   6), S(-36,   6), S(-23,  -7), S(-28,  14), S( 11,  15), S( 20,   6), S(-24, -23),
    S(-38,  -4), S(-40,  -1), S(-22,  -7), S(-23, -10), S(-13,  -2), S(-25,   4), S(  2, -17), S(-21, -18),
    S(-31,  12), S(-32,   5), S(-16, -18), S( -2, -24), S( -1, -23), S(  0, -13), S(-16,  -7), S(-24, -14),
    S(-17,  34), S(-25,  11), S(-12,  -4), S( -2, -29), S(  3, -17), S( 25, -14), S( -9,   1), S(-14,  13),
    S(  9,  49), S(-11,  28), S( 10,   3), S( 20, -19), S( 37,   2), S( 85,  18), S( 32,  33), S( 21,  41),
    S( 93,  -1), S( 37,  -6), S( 52,  -9), S( 79, -28), S( 89,  -5), S( 39,  10), S(-42,  19), S(-53,  26)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -37), S( -16,  -38), S( -15,  -14), S(  -9,    2),
    S( -11,  -21), S(  -5,   -3), S(   5,  -16), S(   4,   -3),
    S(   7,  -32), S(  13,   -7), S(  23,   -7), S(  19,   20),
    S(  17,    2), S(  18,   19), S(  44,   22), S(  30,   38),
    S(  47,   13), S(  47,   12), S(  60,   21), S(  58,   41),
    S(  -7,    6), S(  29,    8), S(  58,   24), S(  59,   24),
    S(  -8,   -6), S( -42,   16), S(  34,    4), S(  36,   31),
    S(-166,  -91), S(-112,    9), S(-155,   14), S(  19,    9)
};

const Scorepair BishopSQT[32] = {
    S(  31,  -35), S(  26,  -11), S(  -4,   -6), S(   2,   -9),
    S(  38,  -40), S(  40,  -26), S(  36,  -13), S(  15,    3),
    S(  27,   -9), S(  38,   -2), S(  24,   -7), S(  20,   27),
    S(  11,  -24), S(  21,    5), S(  19,   19), S(  24,   27),
    S(   9,   -2), S(  14,   15), S(  27,   18), S(  26,   33),
    S(  43,    5), S(  19,   28), S(  17,   10), S(  36,   10),
    S( -58,    6), S( -57,   -2), S(  -2,   15), S( -15,   13),
    S( -48,  -14), S( -61,   17), S(-152,   18), S(-131,   11)
};

const Scorepair RookSQT[32] = {
    S(  -7,  -29), S(  -8,  -20), S(  -8,  -10), S(  -2,  -19),
    S( -32,  -23), S( -19,  -25), S(  -7,  -11), S( -10,  -12),
    S( -29,  -18), S(  -5,  -15), S( -25,   -1), S( -18,   -1),
    S( -22,   -6), S( -20,    5), S( -25,    9), S(  -9,    1),
    S(  -5,    9), S(   5,   15), S(  18,    9), S(  29,    2),
    S( -13,   20), S(  24,   10), S(  25,   11), S(  53,    1),
    S(  11,   21), S(  -5,   24), S(  35,   24), S(  45,   25),
    S(  15,   30), S(  13,   34), S(  10,   36), S(  14,   32)
};

const Scorepair QueenSQT[32] = {
    S(  17,  -71), S(   1,  -70), S(  19,  -89), S(  27,  -67),
    S(  23,  -66), S(  23,  -61), S(  36,  -57), S(  23,  -11),
    S(  18,  -45), S(  26,  -19), S(  15,   26), S(  12,   17),
    S(  17,   -7), S(  20,   24), S(   2,   37), S(  -8,   64),
    S(  23,    6), S(  -4,   49), S(   8,   39), S( -16,   72),
    S(  11,    4), S(  -7,   38), S( -15,   62), S(  -5,   58),
    S( -11,    3), S( -42,   12), S( -26,   67), S( -44,   88),
    S( -52,   23), S( -16,   30), S( -17,   54), S( -11,   60)
};

const Scorepair KingSQT[32] = {
    S(  64, -119), S(  62,  -57), S( -24,  -41), S( -30,  -55),
    S(  57,  -52), S(  18,  -18), S( -14,   -5), S( -53,    1),
    S( -68,  -39), S(  12,  -14), S( -23,   11), S( -28,   22),
    S(-150,  -27), S( -56,    7), S( -37,   29), S( -21,   40),
    S( -89,    4), S(   1,   48), S(  16,   58), S(   2,   59),
    S( -35,   24), S(  55,   75), S(  50,   82), S(  46,   67),
    S( -45,  -15), S(  12,   61), S(  41,   72), S(  38,   56),
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
