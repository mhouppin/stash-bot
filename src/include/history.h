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

#ifndef HISTORY_H
#define HISTORY_H

#include "core.h"
#include "hashkey.h"

enum {
    HISTORY_MAX = 16384,

    CORRECTION_HISTORY_ENTRY_NB = 16384,
    CORRECTION_HISTORY_GRAIN = 256,
    CORRECTION_HISTORY_WEIGHT_SCALE = 256,
    CORRECTION_HISTORY_MAX = CORRECTION_HISTORY_GRAIN * 32,
};

INLINED i16 history_bonus(u16 depth) {
    const i32 d = (i32)depth;

    return (i16)i32_min(27 * d * d - 32 * d - 34, 2461);
}

INLINED void update_hist_entry(i16 *entry, i16 bonus) {
    *entry += bonus - (i32)*entry * (i32)i32_abs(bonus) / HISTORY_MAX;
}

typedef struct {
    i16 data[COLOR_NB][SQUARE_NB * SQUARE_NB];
} ButterflyHistory;

INLINED void butterfly_hist_update(ButterflyHistory *hist, Piece piece, Move move, i16 bonus) {
    update_hist_entry(&hist->data[piece_color(piece)][move_square_mask(move)], bonus);
}

INLINED i16 butterfly_hist_score(const ButterflyHistory *hist, Piece piece, Move move) {
    return hist->data[piece_color(piece)][move_square_mask(move)];
}

typedef struct {
    i16 data[PIECE_NB][SQUARE_NB];
} PieceHistory;

typedef struct {
    PieceHistory piece_history[PIECE_NB][SQUARE_NB];
} ContinuationHistory;

typedef struct {
    Move data[PIECE_NB][SQUARE_NB];
} CountermoveHistory;

INLINED void piece_hist_update(PieceHistory *hist, Piece piece, Square to, i16 bonus) {
    update_hist_entry(&hist->data[piece][to], bonus);
}

INLINED i16 piece_hist_score(const PieceHistory *hist, Piece piece, Square to) {
    return hist->data[piece][to];
}

typedef struct {
    i16 data[PIECE_NB][SQUARE_NB][PIECETYPE_NB];
} CaptureHistory;

INLINED void capture_hist_update(
    CaptureHistory *hist,
    Piece piece,
    Square to,
    Piecetype captured,
    i16 bonus
) {
    update_hist_entry(&hist->data[piece][to][captured], bonus);
}

INLINED i16
    capture_hist_score(const CaptureHistory *hist, Piece piece, Square to, Piecetype captured) {
    return hist->data[piece][to][captured];
}

typedef struct {
    i16 data[COLOR_NB][CORRECTION_HISTORY_ENTRY_NB];
} CorrectionHistory;

INLINED void correction_hist_update(
    CorrectionHistory *hist,
    Color stm,
    Key entry_key,
    i16 weight,
    i32 eval_diff
) {
    i16 *entry = &hist->data[stm][entry_key % CORRECTION_HISTORY_ENTRY_NB];
    const i32 scaled_diff = eval_diff * CORRECTION_HISTORY_GRAIN;

    i32 update = (i32)*entry * (i32)(CORRECTION_HISTORY_WEIGHT_SCALE - weight)
        + (i32)scaled_diff * (i32)weight;

    *entry = (i16)i32_clamp(
        update / CORRECTION_HISTORY_WEIGHT_SCALE,
        -CORRECTION_HISTORY_MAX,
        CORRECTION_HISTORY_MAX
    );
}

INLINED i16 correction_hist_score(const CorrectionHistory *hist, Color stm, Key entry_key) {
    return hist->data[stm][entry_key % CORRECTION_HISTORY_ENTRY_NB] / CORRECTION_HISTORY_GRAIN;
}

static_assert(sizeof(ButterflyHistory) % 64 == 0, "Misaligned history");
static_assert(sizeof(ContinuationHistory) % 64 == 0, "Misaligned history");
static_assert(sizeof(CountermoveHistory) % 64 == 0, "Misaligned history");
static_assert(sizeof(CaptureHistory) % 64 == 0, "Misaligned history");
static_assert(sizeof(CorrectionHistory) % 64 == 0, "Misaligned history");

#endif
