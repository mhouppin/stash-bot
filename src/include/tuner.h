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

#ifndef TUNER_H
#define TUNER_H

#include "chess_types.h"
#include "core.h"
#include "evaluate.h"
#include "strmanip.h"

// Checks if the parameter at the current index is a King Safety term or not. This is required
// because KS terms use a quadratic formula for middlegame values.
INLINED bool is_param_safety_term(u16 index) {
    return index >= IDX_KING_SAFETY;
}

// Checks if the parameter is actually activated for the current position.
INLINED bool evaltrace_is_active_param(u16 index) {
    return is_param_safety_term(index) ? Trace.coeffs[index][WHITE] || Trace.coeffs[index][BLACK]
                                       : Trace.coeffs[index][WHITE] != Trace.coeffs[index][BLACK];
}

// Returns the sigmoid of the eval using k as the scaling divisor.
f64 sigmoid(f64 k, f64 eval);

// Interpolates between lo and hi.
f64 lerp(f64 lo, f64 hi, f64 rate);

// Global config for the tuning code.
typedef struct {
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

// Self-explanatory.
void tuner_config_set_default_values(TunerConfig *tuner_config);

// Holds a vector of all the phased parameters.
typedef struct {
    f64 values[IDX_COUNT][2];
} TupleVector;

// Sets back all values in the vector to zero.
void tp_vector_reset(TupleVector *tp_vector);

// Holds the safety scores for each side and game phase.
typedef struct {
    f64 values[COLOR_NB][PHASE_NB];
} TupleSafetyEval;

// A tuple holding a single eval parameter.
typedef struct {
    u16 index;
    i8 wcoeff;
    i8 bcoeff;
} TunerTuple;

// An entry in the tuning dataset.
typedef struct {
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

// Initializes the entry with the given board state.
bool tuner_entry_init(TunerEntry *restrict entry, const Board *restrict board);

// Frees all memory associated with the entry.
void tuner_entry_destroy(TunerEntry *entry);

// Uses the global EvalTrace table to initialize the active eval parameters in the entry.
void tuner_entry_set_tuples_from_evaltrace(TunerEntry *entry);

// Computes the eval with the adjusted eval terms.
f64 tuner_entry_adjusted_eval(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleSafetyEval *restrict safety_eval
);

// Updates the gradient values with the entry.
void tuner_entry_update_gradient(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    f64 sigmoid_k
);

// The dataset holding all tuning entries for the session.
typedef struct {
    TunerEntry *entries;
    usize size;
    usize capacity;
} TunerDataset;

// Initializes the dataset.
void tuner_dataset_init(TunerDataset *tuner_dataset);

// Frees all memory associated with the dataset.
void tuner_dataset_destroy(TunerDataset *tuner_dataset);

// Adds all entries from the given file to the dataset.
void tuner_dataset_add_file(TunerDataset *tuner_dataset, const char *filename);

// Starts the tuning session using the given config.
void tuner_dataset_start_session(TunerDataset *tuner_dataset, const TunerConfig *tuner_config);

// Computes the mean squared error at the start of the tuning session.
f64 tuner_dataset_static_eval_mse(const TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda);

// Determines the sigmoid scaling divisor that minimizes the mean squared error at the start of the
// tuning session.
f64 tuner_dataset_compute_optimal_k(const TunerDataset *tuner_dataset, f64 lambda);

// Blends the game result with the search score for each entry using the given lambda value.
void tuner_dataset_compute_wdl_eval_mix(TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda);

// Computes the mean squared error with adjusted eval terms.
f64 tuner_dataset_adjusted_eval_mse(
    const TunerDataset *restrict tuner_dataset,
    const TupleVector *restrict delta,
    f64 sigmoid_k
);

// Computes the gradient values for the given batch.
void tuner_dataset_compute_gradient(
    const TunerDataset *tuner_dataset,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k,
    usize batch_index
);

// Struct holding the Adam state values.
typedef struct {
    TupleVector gradient;
    TupleVector momentum;
    TupleVector velocity;
} AdamOptimizer;

// Initializes the optimizer at the start of the tuning session.
void adam_init(AdamOptimizer *adam);

// Updates delta using the gradient values and resets back the gradient array to zero.
void adam_update_delta(
    AdamOptimizer *restrict adam,
    TupleVector *restrict delta,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k
);

// Runs a full iteration of the tuning session.
void adam_do_one_iteration(
    AdamOptimizer *restrict adam,
    const TunerDataset *restrict tuner_dataset,
    TupleVector *restrict delta,
    const TunerConfig *tuner_config,
    f64 sigmoid_k
);

// Enum for display value types.
typedef enum {
    TypeScorepair,
    TypeScorepairArray,
    TypeRawString,
    TypeNewline,
    TypeCustom,
} DisplayType;

// Struct holding the information for displaying a scorepair.
typedef struct {
    String name;
    u16 index;
    u16 name_alignment;
    u16 value_padding;
} DisplayScorepair;

// Prints the scorepair with adjusted params on stdout.
void disp_scorepair_show(
    const DisplayScorepair *disp_scorepair,
    const TupleVector *base,
    const TupleVector *delta
);

// Struct holding the information for displaying a scorepair array.
typedef struct {
    String name;
    u16 index;
    u16 array_size;
    u16 value_start;
    u16 value_end;
    u16 value_padding;
    u16 values_per_line;
    bool long_prefix;
} DisplayScorepairArray;

// Prints the scorepair array with adjusted params on stdout.
void disp_sp_array_show(
    const DisplayScorepairArray *disp_sp_array,
    const TupleVector *base,
    const TupleVector *delta
);

// Struct holding the information for displaying a string.
typedef struct {
    String str;
} DisplayRawString;

// Struct holding the information for custom display functions. (This is currently only used for
// material values.)
typedef struct {
    void (*custom_display)(const TupleVector *, const TupleVector *);
} DisplayCustomData;

// The union holding the information for a display.
typedef union {
    DisplayScorepair scorepair;
    DisplayScorepairArray scorepair_array;
    DisplayRawString raw_string;
    DisplayCustomData custom;
} DisplayUnion;

// The struct holding the information for a display.
typedef struct {
    DisplayType block_type;
    DisplayUnion data_union;
} DisplayBlock;

// The struct holding the full eval display that gets periodically printed to stdout.
typedef struct {
    DisplayBlock *blocks;
    usize size;
    usize capacity;
} DisplaySequence;

// Initializes the display sequence.
void disp_sequence_init(DisplaySequence *disp_sequence);

// Frees all memory associated with the display sequence.
void disp_sequence_destroy(DisplaySequence *disp_sequence);

// Increases the capacity of the display sequence if there's no place for an extra block.
void disp_sequence_maybe_extend_allocation(DisplaySequence *disp_sequence);

// Adds a scorepair to the display sequence.
void disp_sequence_add_scorepair(
    DisplaySequence *disp_sequence,
    StringView name,
    u16 index,
    u16 name_alignment,
    u16 value_padding
);

// Adds a scorepair array to the display sequence.
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

// Adds a string to the display sequence.
void disp_sequence_add_raw_string(DisplaySequence *disp_sequence, StringView str);

// Adds a newline to the display sequence.
void disp_sequence_add_newline(DisplaySequence *disp_sequence);

// Adds a custom display to the display sequence.
void disp_sequence_add_custom(
    DisplaySequence *disp_sequence,
    void (*custom_display)(const TupleVector *, const TupleVector *)
);

// Displays all blocks in the display sequence.
void disp_sequence_show(
    const DisplaySequence *restrict disp_sequence,
    const TupleVector *restrict base,
    const TupleVector *restrict delta
);

// Copies the mg/eg values from the scorepair into the base vector.
void base_values_set_scorepair(TupleVector *base, Scorepair value, u16 index);

// Copies all values from the scorepair array into the base vector.
void base_values_set_sp_array(
    TupleVector *restrict base,
    const Scorepair *restrict sp_array,
    u16 index,
    u16 value_start,
    u16 value_end
);

// Helper macros for initializing the display sequence and the base eval parameters at the same
// time. They are only meant to be used in the init_disp_sequence_and_base_values() function.

// Adds a scorepair to the display sequence.
// `name` is the exact name of the constant in the eval.
// `index` is the enum value in the eval.
// `name_alignment` sets the minimum width of the name when printed, padding the name with spaces
//     if necessary.
// `value_padding` sets the minimum width of individual scores in the scorepair, padding them with
//     spaces if necessary.
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

// Adds a scorepair array to the display sequence.
// `name` is the exact name of the constant in the eval.
// `index` is the enum value in the eval.
// `array_size` is the raw size of the array in the eval.
// `value_start` is the index of the first ACTIVE term in the eval. This is done to, for example,
//     allow indexing pawn eval terms by rank in the eval, while keeping the tuple vector compact.
// `value_end` is the index of the last ACTIVE term in the eval PLUS ONE.
// `value_padding` sets the minimum width of individual scores in each scorepair, padding them with
//     spaces if necessary.
// `values_per_line` sets the number of values to fit on each line when displaying the array.
// `long_prefix` sets whether the scorepairs should be using the SPAIR() macro or the S() macro.
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

// Fills the display sequence with all eval terms and copies the current eval params into the tuple
// vector.
void init_disp_sequence_and_base_values(
    DisplaySequence *restrict disp_sequence,
    TupleVector *restrict base
);

#endif
