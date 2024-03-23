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

#ifndef PSQ_SCORE_H
#define PSQ_SCORE_H

#include "types.h"

// Enum for all pieces' midgame and endgame scores
enum
{
    PAWN_MG_SCORE = 114,
    KNIGHT_MG_SCORE = 382,
    BISHOP_MG_SCORE = 409,
    ROOK_MG_SCORE = 558,
    QUEEN_MG_SCORE = 1162,

    PAWN_EG_SCORE = 203,
    KNIGHT_EG_SCORE = 662,
    BISHOP_EG_SCORE = 713,
    ROOK_EG_SCORE = 1153,
    QUEEN_EG_SCORE = 2168,

    PAWN_SEE_SCORE = 100,
    KNIGHT_SEE_SCORE = 318,
    BISHOP_SEE_SCORE = 370,
    ROOK_SEE_SCORE = 542,
    QUEEN_SEE_SCORE = 1087,
};

// Global for the piece values indexed by phase and piece
extern const score_t PieceScores[PHASE_NB][PIECE_NB];

// Global for the PSQT
extern scorepair_t PsqScore[PIECE_NB][SQUARE_NB];

// Initializes the PSQT.
void psq_score_init(void);

#endif // PSQ_SCORE_H
