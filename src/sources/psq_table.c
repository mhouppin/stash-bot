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
    S(-39,   6), S(-18,   5), S(-36,   7), S(-24,  -6), S(-28,  15), S( 10,  16), S( 19,   6), S(-24, -22),
    S(-38,  -4), S(-39,  -2), S(-22,  -7), S(-23, -10), S(-13,  -2), S(-25,   4), S(  2, -17), S(-22, -18),
    S(-31,  11), S(-31,   4), S(-16, -17), S( -2, -23), S( -1, -22), S( -1, -12), S(-16,  -7), S(-25, -14),
    S(-17,  32), S(-25,  10), S(-12,  -4), S( -2, -26), S(  3, -15), S( 24, -13), S( -9,   1), S(-14,  13),
    S(  8,  46), S(-12,  26), S(  9,   1), S( 22, -13), S( 37,   4), S( 83,  19), S( 32,  34), S( 20,  39),
    S( 96,  -4), S( 32,  -8), S( 51,  -8), S( 79, -24), S( 83,  -4), S( 38,  11), S(-27,  16), S(-43,  19)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -50,  -41), S( -15,  -39), S( -13,  -17), S(  -8,    0),
    S( -10,  -24), S(  -4,   -6), S(   6,  -18), S(   5,   -5),
    S(   8,  -34), S(  14,   -8), S(  24,   -8), S(  19,   18),
    S(  18,    0), S(  19,   17), S(  45,   21), S(  31,   36),
    S(  47,   16), S(  47,   12), S(  60,   22), S(  59,   42),
    S(  -5,    8), S(  29,   10), S(  58,   26), S(  59,   27),
    S(  -8,   -2), S( -41,   17), S(  33,    6), S(  34,   33),
    S(-157,  -99), S(-114,   11), S(-176,   19), S(  13,   11)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -38), S(  28,  -16), S(  -3,   -7), S(   4,  -10),
    S(  39,  -43), S(  40,  -27), S(  37,  -15), S(  16,    1),
    S(  28,  -10), S(  38,   -4), S(  24,   -8), S(  21,   24),
    S(  12,  -24), S(  22,    4), S(  20,   17), S(  25,   25),
    S(   9,    1), S(  14,   16), S(  27,   19), S(  27,   33),
    S(  43,    7), S(  19,   29), S(  18,   12), S(  37,   11),
    S( -56,    7), S( -56,    0), S(  -1,   16), S( -14,   15),
    S( -46,  -12), S( -68,   20), S(-155,   19), S(-144,   14)
};

const Scorepair RookSQT[32] = {
    S(  -6,  -33), S(  -7,  -24), S(  -6,  -13), S(  -1,  -23),
    S( -31,  -29), S( -18,  -28), S(  -6,  -14), S(  -9,  -16),
    S( -27,  -22), S(  -5,  -17), S( -24,   -3), S( -17,   -4),
    S( -21,   -7), S( -20,    4), S( -24,    8), S(  -9,    0),
    S(  -6,   14), S(   5,   18), S(  18,   11), S(  29,    5),
    S( -13,   24), S(  24,   12), S(  26,   13), S(  52,    4),
    S(  10,   24), S(  -5,   25), S(  34,   25), S(  45,   26),
    S(   8,   33), S(   4,   37), S(   6,   38), S(   8,   34)
};

const Scorepair QueenSQT[32] = {
    S(  14,  -68), S(  -2,  -64), S(  16,  -85), S(  24,  -64),
    S(  21,  -65), S(  20,  -59), S(  34,  -56), S(  20,   -9),
    S(  16,  -43), S(  24,  -18), S(  13,   25), S(  10,   16),
    S(  15,   -5), S(  18,   26), S(   1,   36), S(  -9,   63),
    S(  21,   11), S(  -5,   49), S(   8,   37), S( -15,   68),
    S(  10,    4), S(  -7,   36), S( -13,   57), S(  -2,   52),
    S( -10,    1), S( -40,    6), S( -25,   63), S( -44,   84),
    S( -54,   26), S( -13,   25), S( -17,   52), S(  -8,   55)
};

const Scorepair KingSQT[32] = {
    S(  68, -120), S(  67,  -58), S( -18,  -43), S( -24,  -58),
    S(  61,  -54), S(  23,  -20), S( -10,   -6), S( -48,   -1),
    S( -63,  -41), S(  17,  -15), S( -20,   11), S( -24,   22),
    S(-168,  -24), S( -69,    8), S( -44,   31), S( -19,   41),
    S(-102,    7), S(  -9,   49), S(  20,   61), S(  14,   61),
    S( -41,   27), S(  56,   75), S(  54,   83), S(  54,   68),
    S( -49,  -13), S(   8,   59), S(  39,   70), S(  38,   55),
    S(  24, -241), S(  98,  -38), S(  72,    1), S(  12,   12)
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
