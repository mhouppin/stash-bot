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

#ifndef HISTORY_H
#define HISTORY_H

#include "hashkey.h"
#include "types.h"
#include <stdlib.h>

enum
{
    HistoryResolution = 16384
};

enum
{
    CORRECTION_HISTORY_ENTRY_NB = 16384,
    CORRECTION_HISTORY_GRAIN = 256,
    CORRECTION_HISTORY_WEIGHT_SCALE = 256,
    CORRECTION_HISTORY_MAX = CORRECTION_HISTORY_GRAIN * 32,
};

typedef int16_t butterfly_history_t[COLOR_NB][SQUARE_NB * SQUARE_NB];
typedef int16_t piece_history_t[PIECE_NB][SQUARE_NB];
typedef int16_t capture_history_t[PIECE_NB][SQUARE_NB][PIECETYPE_NB];
typedef piece_history_t continuation_history_t[PIECE_NB][SQUARE_NB];
typedef move_t countermove_history_t[PIECE_NB][SQUARE_NB];
typedef int32_t correction_history_t[COLOR_NB][CORRECTION_HISTORY_ENTRY_NB];

// Returns the history bonus for the given depth.
INLINED int history_bonus(int depth) { return imin(25 * depth * depth - 22 * depth - 28, 2536); }

// Updates the butterfly history table for the given piece and move.
INLINED void add_bf_history(butterfly_history_t hist, piece_t piece, move_t move, int32_t bonus)
{
    int16_t *entry = &hist[piece_color(piece)][square_mask(move)];

    *entry += bonus - (int32_t)*entry * abs(bonus) / HistoryResolution;
}

// Gets the butterfly history bonus for the given piece and move.
INLINED score_t get_bf_history_score(const butterfly_history_t hist, piece_t piece, move_t move)
{
    return hist[piece_color(piece)][square_mask(move)];
}

// Updates the piece history table for the given piece and destination square.
INLINED void add_pc_history(piece_history_t hist, piece_t pc, square_t to, int32_t bonus)
{
    int16_t *entry = &hist[pc][to];

    *entry += bonus - (int32_t)*entry * abs(bonus) / HistoryResolution;
}

// Gets the piece history bonus for the given piece and destination square.
INLINED score_t get_pc_history_score(const piece_history_t hist, piece_t pc, square_t to)
{
    return hist[pc][to];
}

// Updates the capture history table for the given piece, destination square and captured piece.
INLINED void add_cap_history(
    capture_history_t hist, piece_t pc, square_t to, piece_t captured, int32_t bonus)
{
    int16_t *entry = &hist[pc][to][piece_type(captured)];

    *entry += bonus - (int32_t)*entry * abs(bonus) / HistoryResolution;
}

// Gets the capture history bonus for the given piece, destination square and captured piece.
INLINED score_t get_cap_history_score(
    const capture_history_t hist, piece_t pc, square_t to, piece_t captured)
{
    return hist[pc][to][piece_type(captured)];
}

// Updates the correction history table for the given side, pawnkey, update weight and score
// differential.
INLINED void add_correction_history(
    correction_history_t hist, color_t side, hashkey_t pawnKey, int depth, int32_t diff)
{
    int32_t *entry = &hist[side][pawnKey % CORRECTION_HISTORY_ENTRY_NB];
    int32_t newWeight = imin(16, 1 + depth);
    int32_t scaledDiff = diff * CORRECTION_HISTORY_GRAIN;

    int32_t update =
        *entry * (CORRECTION_HISTORY_WEIGHT_SCALE - newWeight) + scaledDiff * newWeight;

    // Clamp entry in-bounds.
    *entry = imax(-CORRECTION_HISTORY_MAX,
        imin(CORRECTION_HISTORY_MAX, update / CORRECTION_HISTORY_WEIGHT_SCALE));
}

// Get the correction history differential for the given side and pawnkey.
INLINED int32_t get_correction(const correction_history_t hist, color_t side, hashkey_t pawnKey)
{
    return hist[side][pawnKey % CORRECTION_HISTORY_ENTRY_NB] / CORRECTION_HISTORY_GRAIN;
}

#endif // HISTORY_H
