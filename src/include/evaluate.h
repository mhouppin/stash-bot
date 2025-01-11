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
    IDX_KNIGHT_CLOSED_POS,
    IDX_KNIGHT_SHIELDED = IDX_KNIGHT_CLOSED_POS + 5,
    IDX_KNIGHT_OUTPOST,
    IDX_BISHOP_PAWNS_COLOR,
    IDX_BISHOP_PAIR = IDX_BISHOP_PAWNS_COLOR + 7,
    IDX_BISHOP_SHIELDED,
    IDX_BISHOP_OUTPOST,
    IDX_BISHOP_LONG_DIAG,
    IDX_ROOK_SEMIOPEN,
    IDX_ROOK_OPEN,
    IDX_ROOK_BLOCKED,
    IDX_ROOK_XRAY_QUEEN,
    IDX_ROOK_TRAPPED,
    IDX_ROOK_BURIED,
    IDX_MOBILITY_KNIGHT,
    IDX_MOBILITY_BISHOP = IDX_MOBILITY_KNIGHT + 9,
    IDX_MOBILITY_ROOK = IDX_MOBILITY_BISHOP + 14,
    IDX_MOBILITY_QUEEN = IDX_MOBILITY_ROOK + 15,
    IDX_BACKWARD = IDX_MOBILITY_QUEEN + 28,
    IDX_STRAGGLER,
    IDX_DOUBLED,
    IDX_ISOLATED,
    IDX_PASSER,
    IDX_PHALANX = IDX_PASSER + 6,
    IDX_DEFENDER = IDX_PHALANX + 6,
    IDX_PASSED_OUR_KING_DIST = IDX_DEFENDER + 5,
    IDX_PASSED_THEIR_KING_DIST = IDX_PASSED_OUR_KING_DIST + 24,
    IDX_PAWN_ATK_MINOR = IDX_PASSED_THEIR_KING_DIST + 24,
    IDX_PAWN_ATK_ROOK,
    IDX_PAWN_ATK_QUEEN,
    IDX_MINOR_ATK_ROOK,
    IDX_MINOR_ATK_QUEEN,
    IDX_ROOK_ATK_QUEEN,
    IDX_HANGING_PAWN,

    // All King Safety terms should go under this enum value. This is done to
    // facilitate gradient calculations in the internal tuner.
    IDX_KING_SAFETY,
    IDX_KS_KNIGHT,
    IDX_KS_BISHOP,
    IDX_KS_ROOK,
    IDX_KS_QUEEN,
    IDX_KS_ATTACK,
    IDX_KS_WEAK_Z,
    IDX_KS_CHECK_N,
    IDX_KS_CHECK_B,
    IDX_KS_CHECK_R,
    IDX_KS_CHECK_Q,
    IDX_KS_UNSAFE_CHECK,
    IDX_KS_QUEENLESS,
    IDX_KS_STORM,
    IDX_KS_SHELTER = IDX_KS_STORM + 24,
    IDX_KS_OFFSET = IDX_KS_SHELTER + 24,
    IDX_COUNT
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
    Trace.coeffs[IDX_KS_KNIGHT][us] = 0;
    Trace.coeffs[IDX_KS_BISHOP][us] = 0;
    Trace.coeffs[IDX_KS_ROOK][us] = 0;
    Trace.coeffs[IDX_KS_QUEEN][us] = 0;
    Trace.coeffs[IDX_KS_ATTACK][us] = 0;
}
#else
INLINED void trace_init(void) {
}

INLINED void trace_add(u16 index, Color us, i8 coeff) {
    (void)index;
    (void)us;
    (void)coeff;
}

INLINED void trace_set_phase(i16 phase) {
    (void)phase;
}

INLINED void trace_set_safety(Color us, Scorepair safety) {
    (void)us;
    (void)safety;
}

INLINED void trace_set_eval(Scorepair eval) {
    (void)eval;
}

INLINED void trace_set_scalefactor(Scalefactor scalefactor) {
    (void)scalefactor;
}

INLINED void trace_clear_safety(Color us) {
    (void)us;
}
#endif

Score evaluate(const Board *board);

#endif
