/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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
# define PSQ_SCORE_H

# include "piece.h"
# include "score.h"
# include "square.h"

enum
{
    PAWN_MG_SCORE = 94,
    KNIGHT_MG_SCORE = 389,
    BISHOP_MG_SCORE = 409,
    ROOK_MG_SCORE = 582,
    QUEEN_MG_SCORE = 1365,

    PAWN_EG_SCORE = 164,
    KNIGHT_EG_SCORE = 511,
    BISHOP_EG_SCORE = 536,
    ROOK_EG_SCORE = 863,
    QUEEN_EG_SCORE = 1521
};

extern const score_t PieceScores[PHASE_NB][PIECE_NB];
extern scorepair_t PsqScore[PIECE_NB][SQUARE_NB];

void psq_score_init(void);

#endif
