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
    S(-40,   6), S(-18,   5), S(-36,   6), S(-24,  -6), S(-29,  14), S( 11,  15), S( 18,   5), S(-25, -22),
    S(-38,  -4), S(-39,  -2), S(-22,  -7), S(-23, -10), S(-13,  -2), S(-24,   4), S(  1, -18), S(-22, -18),
    S(-31,  11), S(-31,   4), S(-16, -17), S( -2, -23), S( -1, -22), S(  0, -12), S(-17,  -7), S(-25, -14),
    S(-17,  32), S(-24,  10), S(-12,  -4), S( -2, -26), S(  3, -16), S( 24, -13), S( -9,   1), S(-15,  12),
    S(  8,  47), S(-12,  26), S(  9,   2), S( 21, -12), S( 37,   5), S( 77,  20), S( 32,  34), S( 19,  40),
    S( 97,  -5), S( 32,  -8), S( 52,  -8), S( 81, -24), S( 81,  -4), S( 35,  11), S(-24,  14), S(-37,  19)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -49,  -43), S( -14,  -39), S( -12,  -17), S(  -7,    0),
    S(  -8,  -24), S(  -2,   -6), S(   7,  -17), S(   6,   -4),
    S(   9,  -34), S(  14,   -8), S(  25,   -7), S(  20,   18),
    S(  18,    0), S(  19,   17), S(  45,   21), S(  31,   35),
    S(  47,   16), S(  47,   13), S(  61,   22), S(  57,   40),
    S(  -5,    8), S(  29,   11), S(  59,   27), S(  55,   26),
    S(  -8,   -2), S( -40,   17), S(  33,    7), S(  33,   32),
    S(-155, -102), S(-116,   11), S(-185,   22), S(   7,   10)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -38), S(  28,  -17), S(  -2,   -7), S(   4,  -10),
    S(  39,  -43), S(  41,  -27), S(  38,  -15), S(  16,    1),
    S(  28,  -11), S(  39,   -4), S(  25,   -7), S(  22,   24),
    S(  12,  -24), S(  22,    4), S(  20,   17), S(  25,   25),
    S(   9,    1), S(  15,   16), S(  27,   19), S(  27,   33),
    S(  42,    7), S(  20,   29), S(  18,   13), S(  35,   11),
    S( -49,    6), S( -55,    0), S(  -2,   16), S( -16,   15),
    S( -46,  -12), S( -72,   21), S(-157,   19), S(-152,   14)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -33), S(  -6,  -23), S(  -6,  -14), S(   0,  -22),
    S( -30,  -29), S( -16,  -28), S(  -7,  -14), S(  -8,  -15),
    S( -27,  -22), S(  -3,  -17), S( -26,   -4), S( -16,   -4),
    S( -21,   -7), S( -18,    4), S( -26,    8), S(  -7,    0),
    S(  -5,   14), S(   5,   18), S(  15,   11), S(  29,    5),
    S( -13,   24), S(  25,   12), S(  23,   13), S(  52,    4),
    S(  11,   24), S(  -5,   24), S(  31,   24), S(  45,   26),
    S(   8,   33), S(   1,   37), S(   1,   37), S(   5,   34)
};

const Scorepair QueenSQT[32] = {
    S(  14,  -67), S(  -1,  -61), S(  17,  -85), S(  25,  -61),
    S(  21,  -63), S(  21,  -58), S(  34,  -55), S(  21,   -7),
    S(  16,  -42), S(  24,  -16), S(  13,   24), S(  11,   19),
    S(  15,   -4), S(  19,   27), S(   1,   33), S(  -9,   65),
    S(  21,   13), S(  -5,   50), S(   7,   33), S( -14,   67),
    S(  10,    5), S(  -7,   36), S( -13,   53), S(  -3,   48),
    S(  -9,    2), S( -39,    7), S( -31,   58), S( -45,   82),
    S( -54,   27), S( -10,   24), S( -21,   47), S( -10,   52)
};

const Scorepair KingSQT[32] = {
    S(  67, -117), S(  66,  -60), S( -20,  -44), S( -26,  -59),
    S(  66,  -52), S(  21,  -22), S( -10,   -7), S( -48,   -2),
    S( -39,  -39), S(  14,  -16), S( -22,   10), S( -25,   21),
    S(-162,  -20), S( -79,    7), S( -50,   30), S( -20,   40),
    S(-107,   10), S( -20,   49), S(  23,   60), S(  24,   60),
    S( -47,   29), S(  57,   74), S(  60,   83), S(  65,   68),
    S( -55,  -10), S(   3,   59), S(  38,   71), S(  39,   54),
    S(  22, -238), S(  87,  -38), S(  64,    0), S(   4,   11)
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
