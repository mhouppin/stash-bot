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

#ifndef PSQ_TABLE_H
#define PSQ_TABLE_H

#include "chess_types.h"

// Enum for all pieces' midgame, endgame and SEE scores
enum {
    PAWN_MG_SCORE = 103,
    KNIGHT_MG_SCORE = 391,
    BISHOP_MG_SCORE = 407,
    ROOK_MG_SCORE = 539,
    QUEEN_MG_SCORE = 1153,

    PAWN_EG_SCORE = 207,
    KNIGHT_EG_SCORE = 667,
    BISHOP_EG_SCORE = 728,
    ROOK_EG_SCORE = 1172,
    QUEEN_EG_SCORE = 2215,

    PAWN_SEE_SCORE = 114,
    KNIGHT_SEE_SCORE = 350,
    BISHOP_SEE_SCORE = 389,
    ROOK_SEE_SCORE = 549,
    QUEEN_SEE_SCORE = 1077,
};

// Global for the piece values indexed by phase and piece
extern const Score PieceScores[PHASE_NB][PIECE_NB];

// Returns a score pair from the PSQT
INLINED Scorepair psq_table(Piece piece, Square square) {
    extern Scorepair PsqTable[PIECE_NB][SQUARE_NB];

    return PsqTable[piece][square];
}

// Initializes the PSQT
void psq_table_init(void);

#endif
