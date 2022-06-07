/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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
#define TUNER_H

#include "evaluate.h"
#include <stddef.h>

#ifdef TUNE

#define THREADS 3
#define ITERS 10000
#define LEARNING_RATE 0.001
#define LR_DROP_ITERS 10000
#define LR_DROP_VALUE 1.0
#define BATCH_SIZE 2048

typedef struct tune_tuple_s
{
    uint16_t index;
    int8_t wcoeff;
    int8_t bcoeff;
} tune_tuple_t;

typedef struct tune_entry_s
{
    int tupleCount;
    score_t staticEval;
    int phase;
    color_t sideToMove;
    scorepair_t eval;
    scorepair_t safety[COLOR_NB];
    double gameResult;
    double scaleFactor;
    double phaseFactors[PHASE_NB];
    tune_tuple_t *tuples;
} tune_entry_t;

typedef struct tune_data_s
{
    tune_entry_t *entries;
    size_t size;
    size_t maxSize;
} tune_data_t;

typedef int tp_array_t[IDX_COUNT];
typedef double tp_vector_t[IDX_COUNT][2];

#endif

void start_tuning_session(const char *filename);

#ifdef TUNE

void init_base_values(tp_vector_t base);
void init_tuner_entries(tune_data_t *data, const char *filename);
bool init_tuner_entry(tune_entry_t *entry, const board_t *board);
void init_tuner_tuples(tune_entry_t *entry);
double compute_optimal_k(const tune_data_t *data);
void compute_gradient(
    const tune_data_t *data, tp_vector_t gradient, const tp_vector_t delta, double K, int batchIdx);
void update_gradient(
    const tune_entry_t *entry, tp_vector_t gradient, const tp_vector_t delta, double K);
double adjusted_eval(
    const tune_entry_t *entry, const tp_vector_t delta, double safetyScores[COLOR_NB][PHASE_NB]);
double static_eval_mse(const tune_data_t *data, double K);
double adjusted_eval_mse(const tune_data_t *data, const tp_vector_t delta, double K);
double sigmoid(double K, double E);
void print_parameters(const tp_vector_t base, const tp_vector_t delta);

#endif

#endif
