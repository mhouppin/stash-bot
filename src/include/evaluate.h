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

#ifndef EVALUATE_H
#define EVALUATE_H

#include <string.h>

#include "board.h"

enum {
    MIDGAME_COUNT = 24,
    ENDGAME_COUNT = 4
};

typedef enum _TuneIndex {
    IDX_PIECE,
    IDX_PSQT = IDX_PIECE + 5,
    IDX_INITIATIVE = IDX_PSQT + 48 + 32 * 5,
    IDX_KING_SAFETY,
    IDX_COUNT,
    // TODO: almost all constants are missing here.
} TuneIndex;

typedef struct _EvalTrace {
    i16 phase;
    Scorepair tapered_eval;
    Scorepair safety_eval[COLOR_NB];
    Scalefactor eg_scalefactor;
    i8 coeffs[IDX_COUNT][COLOR_NB];
} EvalTrace;

typedef struct _EvaluationData {
    Bitboard king_zone[COLOR_NB];
    Bitboard mobility_zone[COLOR_NB];
    Bitboard attacked[COLOR_NB];
    Bitboard attacked2[COLOR_NB];
    Bitboard attacked_by[COLOR_NB][PIECETYPE_NB];
    i32 safety_attackers[COLOR_NB];
    i32 safety_attacks[COLOR_NB];
    Scorepair safety_value[COLOR_NB];
    i32 position_closed;
} EvaluationData;

extern EvalTrace Trace;

#ifdef TUNE
INLINED void trace_init(void) {
    memset(&Trace, 0, sizeof(Trace));
}

INLINED void trace_add(u16 index, Color us, i8 coeff) {
    Trace.coeffs[index][us] += coeff;
}

INLINED void trace_set_phase(i16 phase) {
    Trace.phase = phase;
}

INLINED void trace_set_safety(Color us, Scorepair safety) {
    Trace.safety_eval[us] = safety;
}

INLINED void trace_set_eval(Scorepair eval) {
    Trace.tapered_eval = eval;
}

INLINED void trace_set_scalefactor(Scalefactor scalefactor) {
    Trace.eg_scalefactor = scalefactor;
}

INLINED void trace_clear_safety(Color us) {
    (void)us;
    // TODO: code missing for now
}
#else
// TODO: replace these by functions that do nothing
#define trace_init()
#define trace_add(index, us, coeff)
#define trace_set_phase(phase)
#define trace_set_safety(us, safety)
#define trace_set_eval(eval)
#define trace_set_scalefactor(scalefactor)
#define trace_clear_safety(us)
#endif

Score evaluate(const Board *board);

#endif
