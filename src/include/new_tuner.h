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

#ifndef TUNER_H
#define TUNER_H

#include "new_chess_types.h"
#include "new_core.h"
#include "new_evaluate.h"
#include "new_strmanip.h"

INLINED bool is_param_safety_term(u16 index) {
    return index >= IDX_KING_SAFETY;
}

INLINED bool evaltrace_is_active_param(u16 index) {
    return is_param_safety_term(index) ? Trace.coeffs[index][WHITE] || Trace.coeffs[index][BLACK]
                                       : Trace.coeffs[index][WHITE] != Trace.coeffs[index][BLACK];
}

f64 sigmoid(f64 k, f64 eval);

f64 lerp(f64 lo, f64 hi, f64 rate);

typedef struct _TunerConfig {
    usize threads;
    usize iterations;
    usize display_every;
    usize batch_size;
    f64 lambda;
    f64 learning_rate;
    f64 gamma;
    usize gamma_iterations;
    f64 beta_1;
    f64 beta_2;
} TunerConfig;

void tuner_config_set_default_values(TunerConfig *tuner_config);

typedef struct _TupleVector {
    f64 values[IDX_COUNT][2];
} TupleVector;

void tp_vector_reset(TupleVector *tp_vector);

typedef struct _TupleSafetyEval {
    f64 values[COLOR_NB][PHASE_NB];
} TupleSafetyEval;

typedef struct _TunerTuple {
    u16 index;
    i8 wcoeff;
    i8 bcoeff;
} TunerTuple;

typedef struct _TunerEntry {
    Score static_eval;
    Score search_score;
    Scorepair tapered_eval;
    Scorepair king_safety[COLOR_NB];
    f64 game_result;
    f64 eg_scalefactor;
    i16 phase;
    f64 mg_factor;
    TunerTuple *tuples;
    usize tuple_count;
} TunerEntry;

bool tuner_entry_init(TunerEntry *restrict entry, const Board *restrict board);

void tuner_entry_destroy(TunerEntry *entry);

void tuner_entry_set_tuples_from_evaltrace(TunerEntry *entry);

f64 tuner_entry_adjusted_eval(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleSafetyEval *restrict safety_eval
);

void tuner_entry_update_gradient(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    f64 sigmoid_k
);

typedef struct _TunerDataset {
    TunerEntry *entries;
    usize size;
    usize capacity;
} TunerDataset;

void tuner_dataset_init(TunerDataset *tuner_dataset);

void tuner_dataset_destroy(TunerDataset *tuner_dataset);

void tuner_dataset_add_file(TunerDataset *tuner_dataset, const char *filename);

void tuner_dataset_start_session(TunerDataset *tuner_dataset, const TunerConfig *tuner_config);

f64 tuner_dataset_static_eval_mse(const TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda);

f64 tuner_dataset_compute_optimal_k(const TunerDataset *tuner_dataset, f64 lambda);

void tuner_dataset_compute_wdl_eval_mix(TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda);

f64 tuner_dataset_adjusted_eval_mse(
    const TunerDataset *restrict tuner_dataset,
    const TupleVector *restrict delta,
    f64 sigmoid_k
);

void tuner_dataset_compute_gradient(
    const TunerDataset *tuner_dataset,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k,
    usize batch_index
);

typedef struct _AdamOptimizer {
    TupleVector gradient;
    TupleVector momentum;
    TupleVector velocity;
} AdamOptimizer;

void adam_init(AdamOptimizer *adam);

void adam_update_delta(
    AdamOptimizer *restrict adam,
    TupleVector *restrict delta,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k
);

void adam_do_one_iteration(
    AdamOptimizer *restrict adam,
    const TunerDataset *restrict tuner_dataset,
    TupleVector *restrict delta,
    const TunerConfig *tuner_config,
    f64 sigmoid_k
);

typedef enum _DisplayType {
    TypeScorepair,
    TypeScorepairArray,
    TypeRawString,
    TypeNewline,
    TypeCustom,
} DisplayType;

typedef struct _DisplayScorepair {
    String name;
    u16 index;
    u16 name_alignment;
    u16 value_padding;
} DisplayScorepair;

void disp_scorepair_show(
    const DisplayScorepair *disp_scorepair,
    const TupleVector *base,
    const TupleVector *delta
);

typedef struct _DisplayScorepairArray {
    String name;
    u16 index;
    u16 array_size;
    u16 value_start;
    u16 value_end;
    u16 value_padding;
    u16 values_per_line;
    bool long_prefix;
} DisplayScorepairArray;

void disp_sp_array_show(
    const DisplayScorepairArray *disp_sp_array,
    const TupleVector *base,
    const TupleVector *delta
);

typedef struct _DisplayRawString {
    String str;
} DisplayRawString;

typedef struct _DisplayCustomData {
    void (*custom_display)(const TupleVector *, const TupleVector *);
} DisplayCustomData;

typedef union _DisplayUnion {
    DisplayScorepair scorepair;
    DisplayScorepairArray scorepair_array;
    DisplayRawString raw_string;
    DisplayCustomData custom;
} DisplayUnion;

typedef struct _DisplayBlock {
    DisplayType block_type;
    DisplayUnion data_union;
} DisplayBlock;

typedef struct _DisplaySequence {
    DisplayBlock *blocks;
    usize size;
    usize capacity;
} DisplaySequence;

void disp_sequence_init(DisplaySequence *disp_sequence);

void disp_sequence_destroy(DisplaySequence *disp_sequence);

void disp_sequence_maybe_extend_allocation(DisplaySequence *disp_sequence);

void disp_sequence_add_scorepair(
    DisplaySequence *disp_sequence,
    StringView name,
    u16 index,
    u16 name_alignment,
    u16 value_padding
);

void disp_sequence_add_scorepair_array(
    DisplaySequence *disp_sequence,
    StringView name,
    u16 index,
    u16 array_size,
    u16 value_start,
    u16 value_end,
    u16 value_padding,
    u16 values_per_line,
    bool long_prefix
);

void disp_sequence_add_raw_string(DisplaySequence *disp_sequence, StringView str);

void disp_sequence_add_newline(DisplaySequence *disp_sequence);

void disp_sequence_add_custom(
    DisplaySequence *disp_sequence,
    void (*custom_display)(const TupleVector *, const TupleVector *)
);

void disp_sequence_show(
    const DisplaySequence *restrict disp_sequence,
    const TupleVector *restrict base,
    const TupleVector *restrict delta
);

void base_values_set_scorepair(TupleVector *base, Scorepair value, u16 index);

void base_values_set_sp_array(
    TupleVector *restrict base,
    const Scorepair *restrict sp_array,
    u16 index,
    u16 value_start,
    u16 value_end
);

// Helper macros for initializing the display sequence and the base eval parameters at the same
// time. They are only meant to be used in the init_disp_sequence_and_base_values() function.

#define TUNE_ADD_SCOREPAIR(name, index, name_alignment, value_padding) \
    do { \
        extern const Scorepair name; \
        base_values_set_scorepair(base, name, index); \
        disp_sequence_add_scorepair( \
            disp_sequence, \
            STATIC_STRVIEW(#name), \
            index, \
            name_alignment, \
            value_padding \
        ); \
    } while (0)

#define TUNE_ADD_SP_ARRAY( \
    name, \
    index, \
    array_size, \
    value_start, \
    value_end, \
    value_padding, \
    values_per_line, \
    long_prefix \
) \
    do { \
        extern const Scorepair name[array_size]; \
        base_values_set_sp_array(base, name, index, value_start, value_end); \
        disp_sequence_add_scorepair_array( \
            disp_sequence, \
            STATIC_STRVIEW(#name), \
            index, \
            array_size, \
            value_start, \
            value_end, \
            value_padding, \
            values_per_line, \
            long_prefix \
        ); \
    } while (0)

void init_disp_sequence_and_base_values(
    DisplaySequence *restrict disp_sequence,
    TupleVector *restrict base
);

#endif
