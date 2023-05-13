/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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
    PAWN_MG_SCORE = 85,
    KNIGHT_MG_SCORE = 367,
    BISHOP_MG_SCORE = 388,
    ROOK_MG_SCORE = 521,
    QUEEN_MG_SCORE = 1063,

    PAWN_EG_SCORE = 163,
    KNIGHT_EG_SCORE = 584,
    BISHOP_EG_SCORE = 612,
    ROOK_EG_SCORE = 979,
    QUEEN_EG_SCORE = 1902
};

// Global for the piece values indexed by phase and piece
extern const score_t PieceScores[PHASE_NB][PIECE_NB];

// Global for the PSQT
extern scorepair_t PsqScore[PIECE_NB][SQUARE_NB];

// Initializes the PSQT.
void psq_score_init(void);

#endif // PSQ_SCORE_H
