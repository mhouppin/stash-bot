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
    S(-36,  15), S(-18,  11), S(-40,  11), S(-34, -10), S(-22,  10), S(  8,  17), S( 16,  10), S(-25, -17),
    S(-34,  -1), S(-45,   0), S(-26,  -5), S(-23, -10), S(-11,  -3), S(-27,   2), S(  3, -15), S(-22, -18),
    S(-28,  18), S(-33,  10), S(-14, -14), S( -4, -24), S(  2, -24), S( -1,  -6), S(-11,  -3), S(-19, -10),
    S(-12,  42), S(-14,  18), S( -7,   2), S( -1, -24), S( 15, -12), S( 31,  -5), S( -3,  13), S(-15,  24),
    S(  1,  55), S( -8,  41), S(  2,  11), S( 25,  -3), S( 41,  12), S( 94,  38), S( 41,  43), S( 13,  50),
    S(117,  15), S( 29,   2), S( 56,   4), S( 93,  -7), S( 83,   3), S( 44,  26), S(-25,  28), S(-33,  28)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -68,  -46), S( -14,  -43), S( -22,  -22), S( -12,    1),
    S(  -7,  -17), S(  -4,   -5), S(  11,  -13), S(   5,    1),
    S(   9,  -30), S(  14,   -3), S(  25,   -8), S(  26,   22),
    S(  20,    2), S(  19,   19), S(  49,   34), S(  39,   38),
    S(  51,   25), S(  52,   19), S(  61,   35), S(  56,   50),
    S(   0,    7), S(  30,   23), S(  64,   40), S(  69,   34),
    S(   2,    4), S( -37,   28), S(  42,   19), S(  42,   46),
    S(-152, -102), S(-118,   15), S(-180,   32), S(  10,   28)
};

const Scorepair BishopSQT[32] = {
    S(  16,  -42), S(  27,  -19), S(   2,   -2), S(   3,   -8),
    S(  39,  -39), S(  42,  -29), S(  35,  -10), S(  14,    2),
    S(  29,   -8), S(  42,    3), S(  25,   -1), S(  23,   33),
    S(   9,  -24), S(  23,   16), S(  24,   27), S(  31,   34),
    S(  12,    4), S(  13,   24), S(  37,   32), S(  31,   45),
    S(  50,   10), S(  24,   32), S(  32,   27), S(  42,   17),
    S( -40,   16), S( -53,    6), S(  16,   24), S(  -1,   15),
    S( -35,   -3), S( -62,   23), S(-164,   27), S(-134,   28)
};

const Scorepair RookSQT[32] = {
    S(  -8,  -27), S(   0,  -15), S(  -6,  -14), S(   1,  -12),
    S( -28,  -24), S( -16,  -21), S( -12,  -11), S(  -7,   -9),
    S( -19,  -15), S(   2,   -6), S( -22,    5), S( -16,    7),
    S( -21,    9), S( -11,   20), S( -22,   23), S(  -1,   16),
    S(   0,   36), S(  27,   38), S(  23,   29), S(  33,   22),
    S(  -4,   43), S(  31,   32), S(  42,   32), S(  64,   19),
    S(  12,   48), S(  -4,   43), S(  34,   36), S(  48,   37),
    S(  14,   49), S(   6,   51), S(   7,   56), S(  12,   50)
};

const Scorepair QueenSQT[32] = {
    S(   4,  -73), S(   3,  -77), S(  13,  -97), S(  24,  -64),
    S(  14,  -62), S(  12,  -60), S(  37,  -52), S(  22,    4),
    S(   9,  -41), S(  26,   -1), S(  20,   32), S(  13,   26),
    S(  15,    6), S(  28,   29), S(   2,   46), S(  -6,   88),
    S(  31,   28), S(  -4,   62), S(   4,   37), S( -12,   77),
    S(   6,    9), S(   3,   51), S( -10,   64), S(   5,   64),
    S(   4,   20), S( -48,   16), S( -30,   66), S( -34,   98),
    S( -43,   41), S(   5,   39), S( -19,   49), S(  -6,   51)
};

const Scorepair KingSQT[32] = {
    S(  55, -131), S(  65,  -72), S( -20,  -55), S( -20,  -67),
    S(  63,  -68), S(  19,  -28), S(  -8,  -18), S( -36,  -10),
    S( -42,  -50), S(   4,  -28), S( -14,    8), S(  -3,   22),
    S(-157,  -15), S( -75,   16), S( -40,   39), S(   2,   47),
    S(-115,   20), S( -26,   74), S(  31,   82), S(  51,   80),
    S( -45,   47), S(  63,  104), S(  73,  113), S(  86,   97),
    S( -70,  -12), S(  -2,   82), S(  37,  103), S(  63,   81),
    S(  22, -221), S(  72,  -21), S(  70,   33), S(   0,   39)
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
