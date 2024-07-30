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

#include "new_psq_table.h"

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
    S(-40,  8), S(-22,  9), S(-38,  9), S(-25, -3), S(-29, 20), S( 10, 20), S( 20, 10), S(-26,-20),
    S(-36, -3), S(-42,  4), S(-18, -3), S(-21, -5), S(-12,  3), S(-26,  9), S(  3,-12), S(-21,-16),
    S(-33, 11), S(-33,  3), S(-17,-20), S( -1,-30), S(  2,-25), S( -3,-13), S(-13, -9), S(-26,-15),
    S(-21, 31), S(-30, 10), S(-14, -8), S( -0,-33), S(  7,-21), S( 25,-15), S( -9, -1), S(-14, 12),
    S(  5, 50), S(-17, 28), S(  5,  2), S( 21,-23), S( 43, -4), S(100, 12), S( 46, 28), S( 22, 39),
    S( 87, 13), S( 53,  6), S( 59, -5), S( 86,-35), S( 95,-22), S( 43, -4), S(-67, 21), S(-66, 28)
};

// Square-based piece scoring for evaluation, using a file symmetry
const Scorepair KnightSQT[32] = {
    S( -53, -44), S(  -8, -44), S(  -5, -22), S(   9,  -2),
    S(  -5, -26), S(  -0,  -6), S(  10, -23), S(  16,  -4),
    S(   2, -41), S(  15, -12), S(  26,  -9), S(  30,  20),
    S(  15,   7), S(  18,  23), S(  40,  28), S(  34,  47),
    S(  31,  23), S(  25,  22), S(  45,  30), S(  33,  51),
    S( -29,  14), S(  25,  15), S(  31,  30), S(  48,  31),
    S(  -6, -16), S( -33,   3), S(  41,  -8), S(  46,  18),
    S(-177, -72), S(-109,   6), S(-117,  13), S(  27,   4)
};

const Scorepair BishopSQT[32] = {
    S(  29, -48), S(  26, -23), S(  -3, -11), S(   5, -11),
    S(  34, -42), S(  40, -31), S(  33, -12), S(  14,   5),
    S(  25, -10), S(  35,  -2), S(  22,  -8), S(  19,  32),
    S(  16, -28), S(  22,  10), S(  19,  28), S(  29,  42),
    S(   0,  -5), S(  17,  23), S(  26,  28), S(  24,  50),
    S(  37,  -0), S(  17,  31), S(  25,   7), S(  40,  18),
    S( -63,  -3), S( -67, -12), S( -10,  15), S( -13,  10),
    S( -60, -21), S( -48,  11), S(-143,  14), S(-109,   8)
};

const Scorepair RookSQT[32] = {
    S( -10, -36), S(  -7, -29), S(  -6, -17), S(   2, -28),
    S( -37, -31), S( -21, -34), S(  -6, -20), S(  -6, -22),
    S( -32, -22), S(  -8, -19), S( -25,  -7), S( -16,  -9),
    S( -27,  -6), S( -23,   5), S( -26,   9), S(  -6,  -2),
    S( -17,  15), S(  -2,  23), S(  14,  15), S(  25,   9),
    S( -15,  28), S(  25,  19), S(  27,  21), S(  54,  13),
    S(  11,  29), S(  -8,  32), S(  40,  33), S(  51,  35),
    S(  25,  32), S(  28,  34), S(  16,  36), S(  22,  31)
};

const Scorepair QueenSQT[32] = {
    S(  13, -76), S(   1, -85), S(  19,-103), S(  31, -87),
    S(  18, -68), S(  24, -74), S(  38, -69), S(  26, -27),
    S(  15, -45), S(  27, -23), S(  17,  15), S(  16,   7),
    S(  15,  -2), S(  25,  12), S(   5,  37), S(  -2,  61),
    S(  22,   7), S(  -4,  52), S(   8,  49), S( -12,  76),
    S(   5,  14), S(  -3,  45), S( -13,  73), S(  -4,  73),
    S( -15,  19), S( -50,  41), S( -15,  72), S( -29,  92),
    S( -33,  24), S( -19,  40), S( -14,  59), S( -16,  66)
};

const Scorepair KingSQT[32] = {
    S(  39,-120), S(  44, -57), S( -36, -43), S( -29, -62),
    S(  34, -52), S(   3, -20), S( -19,  -6), S( -49,  -1),
    S( -71, -46), S(   4, -17), S( -19,   8), S( -18,  20),
    S(-125, -34), S( -32,   0), S( -20,  25), S( -22,  38),
    S( -74,  -0), S(  14,  47), S(  10,  57), S( -10,  59),
    S( -28,  26), S(  55,  82), S(  47,  86), S(  37,  70),
    S( -40,  -7), S(  17,  72), S(  44,  73), S(  38,  58),
    S(  26,-238), S( 105, -25), S(  77,   6), S(  17,  18)
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
