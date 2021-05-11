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

#ifndef TUNER_H
# define TUNER_H

# include <stddef.h>
# include "board.h"

# define ITERS 10000
# define LEARNING_RATE 0.1
# define LR_DROP_ITERS 250
# define LR_DROP_VALUE 1.04
# define BATCH_SIZE 32768

typedef enum tune_idx_e
{
    IDX_PIECE,
    IDX_PSQT = IDX_PIECE + 5,
    IDX_CASTLING = IDX_PSQT + 48 + 64 * 4,
    IDX_INITIATIVE,
    IDX_KS_KNIGHT,
    IDX_KS_BISHOP,
    IDX_KS_ROOK,
    IDX_KS_QUEEN,
    IDX_KS_OFFSET,
    IDX_KNIGHT_SHIELDED,
    IDX_KNIGHT_OUTPOST,
    IDX_KNIGHT_CENTER_OUTPOST,
    IDX_KNIGHT_SOLID_OUTPOST,
    IDX_BISHOP_PAIR,
    IDX_BISHOP_SHIELDED,
    IDX_ROOK_SEMIOPEN,
    IDX_ROOK_OPEN,
    IDX_ROOK_XRAY_QUEEN,
    IDX_MOBILITY_KNIGHT,
    IDX_MOBILITY_BISHOP = IDX_MOBILITY_KNIGHT + 9,
    IDX_MOBILITY_ROOK = IDX_MOBILITY_BISHOP + 14,
    IDX_MOBILITY_QUEEN = IDX_MOBILITY_ROOK + 15,
    IDX_BACKWARD = IDX_MOBILITY_QUEEN + 28,
    IDX_STRAGGLER,
    IDX_DOUBLED,
    IDX_ISOLATED,
    IDX_PASSER,
    IDX_COUNT = IDX_PASSER + 6
}
tune_idx_t;

typedef struct tune_tuple_s
{
    uint16_t index;
    int8_t wcoeff;
    int8_t bcoeff;
}
tune_tuple_t;

typedef struct tune_entry_s
{
    int tupleCount;
    score_t staticEval;
    int phase;
    bool sideToMove;
    scorepair_t eval;
    scorepair_t safety[COLOR_NB];
    double gameResult;
    double scaleFactor;
    double phaseFactors[PHASE_NB];
    tune_tuple_t *tuples;
}
tune_entry_t;

typedef struct tune_data_s
{
    tune_entry_t *entries;
    size_t size;
    size_t maxSize;
}
tune_data_t;

typedef struct gradient_data_s
{
    double endgameEval;
    double wSafetyMg, bsafetyMg;
    double wSafetyEg, bsafetyEg;
}
gradient_data_t;

typedef int tp_array_t[IDX_COUNT];
typedef double tp_vector_t[IDX_COUNT][2];

void start_tuning_session(const char *filename);
void init_base_values(tp_vector_t base);
void init_methods(tp_array_t methods);
void init_tuner_entries(tune_data_t *data, const char *filename);
void init_tuner_entry(tune_entry_t *entry, const board_t *board, const tp_array_t methods);
void init_tuner_tuples(tune_entry_t *entry, const tp_vector_t coeffs, const tp_array_t methods);
double compute_optimal_k(const tune_data_t *data);
void compute_gradient(const tune_data_t *data, tp_vector_t gradient, const tp_vector_t delta,
    const tp_array_t methods, double K, int batchIdx);
double adjusted_eval(const tune_entry_t *entry, const tp_vector_t delta, const tp_array_t methods,
    const gradient_data_t *data);
double static_eval_mse(const tune_data_t *data, double K);
void adjusted_eval_mse(const tune_data_t *data, const tp_vector_t delta, const tp_array_t methods, double K);
void print_parameters(const tp_vector_t base, const tp_vector_t delta);

#endif