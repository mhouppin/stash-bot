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
    S(-42,  14), S(-16,   7), S(-35,   8), S(-28,  -9), S(-28,  14), S(  5,  14), S( 19,   8), S(-20, -18),
    S(-35,  -3), S(-43,   1), S(-24,  -7), S(-23,  -9), S(-15,  -4), S(-28,   2), S(  3, -20), S(-19, -18),
    S(-27,  12), S(-35,   5), S(-15, -18), S( -1, -26), S(  2, -24), S(  2, -10), S(-19, -10), S(-26, -14),
    S(-17,  33), S(-20,  15), S(-10,  -4), S( -4, -29), S(  9, -17), S( 23, -13), S(-10,   3), S(-13,  13),
    S(  8,  49), S(-10,  31), S( 11,   5), S( 22, -15), S( 38,   3), S( 85,  19), S( 32,  34), S( 21,  42),
    S( 93,   1), S( 37,  -4), S( 52,  -8), S( 79, -26), S( 89,  -5), S( 39,  11), S(-42,  19), S(-53,  27)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -51,  -37), S( -12,  -38), S( -17,  -16), S(  -9,    1),
    S( -12,  -21), S(  -6,   -3), S(   3,  -17), S(   3,   -5),
    S(   7,  -32), S(  11,   -8), S(  25,   -9), S(  24,   21),
    S(  15,    2), S(  20,   19), S(  50,   24), S(  29,   40),
    S(  46,   13), S(  49,   14), S(  58,   22), S(  56,   41),
    S(  -7,    6), S(  29,    8), S(  59,   25), S(  61,   26),
    S(  -9,   -6), S( -42,   16), S(  36,    5), S(  36,   31),
    S(-165,  -91), S(-112,    9), S(-155,   14), S(  19,    9)
};

const Scorepair BishopSQT[32] = {
    S(  32,  -35), S(  26,  -11), S(   2,   -7), S(   3,  -10),
    S(  40,  -40), S(  38,  -27), S(  37,  -13), S(   5,    0),
    S(  28,  -10), S(  41,   -2), S(  26,   -5), S(  18,   27),
    S(  12,  -23), S(  21,    4), S(  22,   22), S(  26,   30),
    S(   8,   -2), S(  12,   16), S(  28,   20), S(  27,   35),
    S(  42,    5), S(  20,   29), S(  18,   11), S(  37,   10),
    S( -58,    6), S( -57,   -2), S(  -2,   15), S( -14,   13),
    S( -48,  -14), S( -61,   17), S(-152,   18), S(-131,   12)
};

const Scorepair RookSQT[32] = {
    S(  -5,  -31), S( -10,  -20), S(  -9,  -10), S(  -1,  -19),
    S( -32,  -23), S( -19,  -25), S(  -7,  -12), S( -10,  -12),
    S( -29,  -17), S(  -5,  -14), S( -25,   -2), S( -18,   -2),
    S( -22,   -5), S( -20,    5), S( -25,    8), S(  -8,    2),
    S(  -4,   10), S(   5,   17), S(  18,   10), S(  30,    4),
    S( -13,   20), S(  25,   12), S(  25,   11), S(  54,    1),
    S(  12,   22), S(  -4,   27), S(  35,   26), S(  46,   28),
    S(  15,   30), S(  13,   35), S(  10,   37), S(  15,   32)
};

const Scorepair QueenSQT[32] = {
    S(  16,  -71), S(   0,  -70), S(  16,  -89), S(  22,  -69),
    S(  22,  -67), S(  21,  -62), S(  35,  -58), S(  25,  -10),
    S(  18,  -45), S(  28,  -19), S(  18,   28), S(  15,   18),
    S(  16,   -7), S(  21,   24), S(   3,   38), S(  -8,   65),
    S(  23,    6), S(  -3,   50), S(   9,   39), S( -15,   73),
    S(  12,    4), S(  -7,   38), S( -14,   63), S(  -6,   58),
    S( -10,    3), S( -43,   12), S( -25,   68), S( -44,   88),
    S( -52,   23), S( -16,   30), S( -17,   54), S( -11,   60)
};

const Scorepair KingSQT[32] = {
    S(  57, -123), S(  60,  -60), S( -24,  -45), S( -29,  -56),
    S(  57,  -52), S(  22,  -16), S( -13,   -6), S( -52,   -1),
    S( -69,  -40), S(  13,  -13), S( -23,    8), S( -28,   21),
    S(-150,  -27), S( -56,    8), S( -37,   29), S( -21,   43),
    S( -89,    5), S(   1,   49), S(  16,   60), S(   2,   60),
    S( -35,   24), S(  55,   76), S(  50,   84), S(  46,   69),
    S( -45,  -15), S(  12,   61), S(  41,   73), S(  38,   56),
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
