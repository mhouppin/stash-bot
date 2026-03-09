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
    S(-36,  11), S(-18,   6), S(-38,   8), S(-32,  -8), S(-21,   9), S(  7,  13), S( 15,   5), S(-25, -20),
    S(-35,  -4), S(-44,  -3), S(-27,  -5), S(-23,  -9), S(-11,  -4), S(-23,  -1), S(  3, -16), S(-22, -19),
    S(-29,  13), S(-34,   6), S(-14, -17), S( -6, -23), S(  0, -26), S(  0,  -9), S(-11,  -7), S(-19, -12),
    S(-14,  34), S(-15,  13), S( -8,  -3), S( -3, -23), S( 12, -16), S( 27, -10), S( -4,   7), S(-16,  17),
    S( -2,  43), S(-11,  33), S( -1,   5), S( 25,  -5), S( 39,   8), S( 87,  28), S( 39,  31), S( 10,  41),
    S(111,   4), S( 27,  -9), S( 50,  -7), S( 87, -13), S( 81,  -4), S( 46,  17), S(-24,  18), S(-35,  19)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -63,  -44), S( -12,  -40), S( -20,  -19), S( -11,    4),
    S(  -7,  -17), S(  -2,   -3), S(  12,  -10), S(   5,    0),
    S(  10,  -29), S(  13,   -5), S(  24,  -10), S(  23,   18),
    S(  19,   -2), S(  18,   15), S(  46,   28), S(  37,   31),
    S(  49,   19), S(  50,   14), S(  56,   28), S(  53,   43),
    S(  -2,    3), S(  27,   17), S(  59,   33), S(  65,   27),
    S(   0,    1), S( -40,   22), S(  35,   13), S(  38,   39),
    S(-158, -106), S(-119,   14), S(-183,   28), S(   5,   24)
};

const Scorepair BishopSQT[32] = {
    S(  17,  -44), S(  27,  -17), S(   3,   -3), S(   4,   -8),
    S(  38,  -38), S(  40,  -30), S(  33,  -11), S(  14,   -1),
    S(  27,  -10), S(  39,    1), S(  25,   -4), S(  21,   27),
    S(   9,  -25), S(  21,   11), S(  23,   21), S(  29,   27),
    S(  12,    2), S(  13,   20), S(  33,   27), S(  28,   37),
    S(  46,    6), S(  19,   25), S(  28,   22), S(  38,   11),
    S( -40,   16), S( -51,    4), S(  14,   20), S(  -3,   11),
    S( -38,   -7), S( -64,   22), S(-163,   24), S(-134,   25)
};

const Scorepair RookSQT[32] = {
    S(  -9,  -33), S(  -1,  -18), S(  -9,  -21), S(  -1,  -18),
    S( -26,  -25), S( -15,  -23), S( -12,  -16), S(  -7,  -14),
    S( -20,  -18), S(   2,  -11), S( -21,    0), S( -17,    1),
    S( -23,    3), S( -11,   11), S( -21,   14), S(  -3,    7),
    S(  -1,   24), S(  25,   24), S(  21,   17), S(  31,   14),
    S(  -7,   30), S(  28,   19), S(  38,   21), S(  59,   11),
    S(   6,   34), S(  -8,   30), S(  31,   25), S(  41,   28),
    S(  11,   41), S(   1,   43), S(   4,   46), S(  10,   42)
};

const Scorepair QueenSQT[32] = {
    S(   6,  -71), S(   7,  -72), S(  16,  -92), S(  23,  -65),
    S(  14,  -61), S(  14,  -60), S(  36,  -53), S(  20,   -2),
    S(   9,  -43), S(  24,   -4), S(  19,   28), S(  12,   16),
    S(  13,   -1), S(  26,   22), S(   1,   38), S(  -7,   74),
    S(  30,   23), S(  -4,   55), S(   3,   29), S( -13,   66),
    S(   7,    3), S(   1,   44), S( -13,   55), S(   2,   55),
    S(   1,   14), S( -48,   10), S( -30,   59), S( -35,   89),
    S( -48,   32), S(   2,   35), S( -23,   41), S( -11,   43)
};

const Scorepair KingSQT[32] = {
    S(  59, -121), S(  63,  -65), S( -17,  -44), S( -18,  -57),
    S(  63,  -60), S(  18,  -22), S( -11,  -15), S( -38,   -7),
    S( -38,  -40), S(   3,  -24), S( -19,    9), S(  -8,   20),
    S(-152,  -14), S( -73,   14), S( -43,   34), S(  -3,   39),
    S(-113,   17), S( -23,   64), S(  27,   66), S(  45,   67),
    S( -48,   39), S(  57,   90), S(  64,   95), S(  79,   80),
    S( -69,  -17), S(  -4,   73), S(  33,   90), S(  58,   68),
    S(  26, -223), S(  70,  -28), S(  67,   27), S(  -3,   32)
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
