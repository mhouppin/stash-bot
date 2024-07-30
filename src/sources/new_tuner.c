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

#include "new_tuner.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "new_syncio.h"
#include "new_wmalloc.h"

f64 sigmoid(f64 k, f64 eval) {
    return 1.0 / (1.0 + exp(-eval * k));
}

void tuner_config_set_default_values(TunerConfig *tuner_config) {
    *tuner_config = (TunerConfig) {
        .threads = 1,
        .iterations = 10000,
        .display_every = 50,
        .batch_size = 4096,
        .lambda = 1.0,
        .learning_rate = 0.001,
        .gamma = 1.0,
        .gamma_iterations = 1000,
        .beta_1 = 0.9,
        .beta_2 = 0.999,
    };
}

void tp_vector_reset(TupleVector *tp_vector) {
    for (usize i = 0; i < IDX_COUNT; ++i) {
        tp_vector->values[i][0] = 0.0;
        tp_vector->values[i][1] = 0.0;
    }
}

bool tuner_entry_init(TunerEntry *restrict entry, const Board *restrict board) {
    entry->static_eval = evaluate(board);

    // Don't set up entries for drawn endgames or endgame specializations.
    if (Trace.eg_scalefactor == 0) {
        return false;
    }

    if (board->side_to_move == BLACK) {
        entry->static_eval = -entry->static_eval;
    }

    entry->phase = Trace.phase;
    entry->mg_factor = (entry->phase - ENDGAME_COUNT) / (f64)(MIDGAME_COUNT - ENDGAME_COUNT);
    entry->tapered_eval = Trace.tapered_eval;
    entry->king_safety[WHITE] = Trace.safety_eval[WHITE];
    entry->king_safety[BLACK] = Trace.safety_eval[BLACK];
    entry->eg_scalefactor = Trace.eg_scalefactor / (f64)SCALE_RESOLUTION;
    tuner_entry_set_tuples_from_evaltrace(entry);
    return true;
}

void tuner_entry_destroy(TunerEntry *entry) {
    free(entry->tuples);
}

void tuner_entry_set_tuples_from_evaltrace(TunerEntry *entry) {
    usize length = 0;
    usize tuple_index = 0;

    // Skip parameters which will not influcence gradients during backpropagation to reduce
    // computations (that is, parameters which are inactive for both sides, and additionally linear
    // parameters which have identical activation coefficients for White and Black).
    for (u16 i = 0; i < IDX_COUNT; ++i) {
        length += evaltrace_is_active_param(i);
    }

    entry->tuple_count = length;
    entry->tuples = wrap_malloc(sizeof(TunerTuple) * length);

    for (u16 i = 0; i < IDX_COUNT; ++i) {
        if (evaltrace_is_active_param(i)) {
            entry->tuples[tuple_index++] = (TunerTuple) {
                .index = i,
                .wcoeff = Trace.coeffs[i][WHITE],
                .bcoeff = Trace.coeffs[i][BLACK],
            };
        }
    }
}

f64 tuner_entry_adjusted_eval(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleSafetyEval *restrict safety_eval
) {
    // These variables will hold the value differences of the eval components coming from the delta
    // vector.
    f64 linear_delta[PHASE_NB] = {0.0, 0.0};
    f64 wsafety_delta[PHASE_NB] = {0.0, 0.0};
    f64 bsafety_delta[PHASE_NB] = {0.0, 0.0};

    for (usize i = 0; i < entry->tuple_count; ++i) {
        const u16 index = entry->tuples[i].index;
        const i8 wcoeff = entry->tuples[i].wcoeff;
        const i8 bcoeff = entry->tuples[i].bcoeff;

        if (!is_param_safety_term(index)) {
            linear_delta[MIDGAME] += (wcoeff - bcoeff) * delta->values[index][MIDGAME];
            linear_delta[ENDGAME] += (wcoeff - bcoeff) * delta->values[index][ENDGAME];
        } else {
            wsafety_delta[MIDGAME] += wcoeff * delta->values[index][MIDGAME];
            wsafety_delta[ENDGAME] += wcoeff * delta->values[index][ENDGAME];
            bsafety_delta[MIDGAME] += bcoeff * delta->values[index][MIDGAME];
            bsafety_delta[ENDGAME] += bcoeff * delta->values[index][ENDGAME];
        }
    }

    // These variables will hold the adjusted values of the eval components.
    f64 linear_adj[PHASE_NB];
    f64 wsafety_adj[PHASE_NB];
    f64 bsafety_adj[PHASE_NB];
    f64 safety_adj[PHASE_NB];

    linear_adj[MIDGAME] = (f64)scorepair_midgame(entry->tapered_eval) + linear_delta[MIDGAME];
    linear_adj[ENDGAME] = (f64)scorepair_endgame(entry->tapered_eval) + linear_delta[ENDGAME];

    // Grab the original safety evaluations and add the modified parameters.
    wsafety_adj[MIDGAME] =
        (f64)scorepair_midgame(entry->king_safety[WHITE]) + wsafety_delta[MIDGAME];
    wsafety_adj[ENDGAME] =
        (f64)scorepair_endgame(entry->king_safety[WHITE]) + wsafety_delta[ENDGAME];
    bsafety_adj[MIDGAME] =
        (f64)scorepair_midgame(entry->king_safety[BLACK]) + bsafety_delta[MIDGAME];
    bsafety_adj[ENDGAME] =
        (f64)scorepair_endgame(entry->king_safety[BLACK]) + bsafety_delta[ENDGAME];

    // Remove the original safety evaluation from the linear eval.
    linear_adj[MIDGAME] -= i16_max(0, scorepair_midgame(entry->king_safety[WHITE]))
            * scorepair_midgame(entry->king_safety[WHITE]) / 256
        - i16_max(0, scorepair_midgame(entry->king_safety[BLACK]))
            * scorepair_midgame(entry->king_safety[BLACK]) / 256;
    linear_adj[ENDGAME] -= i16_max(0, scorepair_endgame(entry->king_safety[WHITE])) / 16
        - i16_max(0, scorepair_endgame(entry->king_safety[BLACK])) / 16;

    // Compute the new safety evaluations for each side.
    safety_adj[MIDGAME] = fmax(0.0, wsafety_adj[MIDGAME]) * wsafety_adj[MIDGAME] / 256.0
        - fmax(0.0, bsafety_adj[MIDGAME]) * bsafety_adj[MIDGAME] / 256.0;
    safety_adj[ENDGAME] =
        fmax(0.0, wsafety_adj[ENDGAME]) / 16.0 - fmax(0.0, bsafety_adj[ENDGAME]) / 16.0;

    // Save the safety scores for computing gradients later.
    safety_eval->values[WHITE][MIDGAME] = wsafety_adj[MIDGAME];
    safety_eval->values[WHITE][ENDGAME] = wsafety_adj[ENDGAME];
    safety_eval->values[BLACK][MIDGAME] = bsafety_adj[MIDGAME];
    safety_eval->values[BLACK][ENDGAME] = bsafety_adj[ENDGAME];

    const f64 midgame_adj = linear_adj[MIDGAME] + safety_adj[MIDGAME];
    const f64 endgame_adj = linear_adj[ENDGAME] + safety_adj[ENDGAME];

    return midgame_adj * entry->mg_factor
        + endgame_adj * (1.0 - entry->mg_factor) * entry->eg_scalefactor;
}

void tuner_entry_update_gradient(
    const TunerEntry *restrict entry,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    f64 sigmoid_k
) {
    TupleSafetyEval safety_eval;
    const f64 adjusted_eval = tuner_entry_adjusted_eval(entry, delta, &safety_eval);
    const f64 predict = sigmoid(sigmoid_k, adjusted_eval);
    const f64 error = (entry->game_result - predict) * predict * (1.0 - predict);
    const f64 mg_error = error * entry->mg_factor;
    const f64 eg_error = error * (1.0 - entry->mg_factor);

    const f64 wsafety_mg = fmax(safety_eval.values[WHITE][MIDGAME], 0);
    const f64 bsafety_mg = fmax(safety_eval.values[BLACK][MIDGAME], 0);
    const f64 wsafety_eg = safety_eval.values[WHITE][ENDGAME] > 0.0 ? 1.0 : 0.0;
    const f64 bsafety_eg = safety_eval.values[BLACK][ENDGAME] > 0.0 ? 1.0 : 0.0;

    for (usize i = 0; i < entry->tuple_count; ++i) {
        const u16 index = entry->tuples[i].index;
        const i8 wcoeff = entry->tuples[i].wcoeff;
        const i8 bcoeff = entry->tuples[i].bcoeff;

        if (!is_param_safety_term(index)) {
            gradient->values[index][MIDGAME] += mg_error * (wcoeff - bcoeff);
            gradient->values[index][ENDGAME] +=
                eg_error * (wcoeff - bcoeff) * entry->eg_scalefactor;
        } else {
            gradient->values[index][MIDGAME] +=
                mg_error / 128.0 * (wsafety_mg * wcoeff - bsafety_mg * bcoeff);
            gradient->values[index][ENDGAME] += eg_error / 16.0
                * (wsafety_eg * wcoeff - bsafety_eg * bcoeff) * entry->eg_scalefactor;
        }
    }
}

void tuner_dataset_init(TunerDataset *tuner_dataset) {
    tuner_dataset->entries = NULL;
    tuner_dataset->size = 0;
    tuner_dataset->capacity = 0;
}

void tuner_dataset_destroy(TunerDataset *tuner_dataset) {
    for (usize i = 0; i < tuner_dataset->size; ++i) {
        tuner_entry_destroy(&tuner_dataset->entries[i]);
    }

    free(tuner_dataset->entries);
}

void tuner_dataset_add_file(TunerDataset *tuner_dataset, const char *filename) {
    FILE *f = fopen(filename, "r");
    const usize old_size = tuner_dataset->size;

    if (f == NULL) {
        perror("Unable to open dataset");
        exit(EXIT_FAILURE);
    }

    Board board;
    Boardstack stack;
    String line;
    usize skipped = 0;

    string_init(&line);

    for (usize lineno = 0; string_getline(f, &line) != 0; ++lineno) {
        StringView line_view = strview_from_string(&line);

        if (tuner_dataset->size == tuner_dataset->capacity) {
            tuner_dataset->capacity += !tuner_dataset->capacity ? 16 : tuner_dataset->capacity / 4;
            tuner_dataset->entries =
                wrap_realloc(tuner_dataset->entries, sizeof(TunerEntry) * tuner_dataset->capacity);
        }

        TunerEntry *entry = &tuner_dataset->entries[tuner_dataset->size];

        usize space_index = strview_rfind(line_view, ' ');

        if (space_index == NPOS) {
            fprintf(stderr, "%s:%u: Invalid dataset format\n", filename, (u32)lineno);
            exit(EXIT_FAILURE);
        }

        StringView token =
            strview_trim_whitespaces(strview_subview(line_view, space_index + 1, line_view.size));
        i64 value;

        if (!strview_parse_i64(token, &value)) {
            fprintf(
                stderr,
                "%s:%u:%u: Failed to parse search score\n",
                filename,
                (u32)lineno,
                (u32)space_index + 1
            );
            exit(EXIT_FAILURE);
        }

        entry->search_score = value;
        line_view = strview_subview(line_view, 0, space_index);
        space_index = strview_find_last_not_of_strview(line_view, STATIC_STRVIEW(" "));

        if (space_index == NPOS) {
            fprintf(
                stderr,
                "%s:" FORMAT_LARGE_INT ": Invalid dataset format\n",
                filename,
                (LargeInt)lineno
            );
            exit(EXIT_FAILURE);
        }

        line_view = strview_subview(line_view, 0, space_index + 1);
        space_index = strview_rfind(line_view, ' ');

        if (space_index == NPOS) {
            fprintf(stderr, "%s:%u: Invalid dataset format\n", filename, (u32)lineno);
            exit(EXIT_FAILURE);
        }

        token =
            strview_trim_whitespaces(strview_subview(line_view, space_index + 1, line_view.size));

        if (strview_equals_strview(token, STATIC_STRVIEW("0.0"))) {
            entry->game_result = 0.0;
        } else if (strview_equals_strview(token, STATIC_STRVIEW("0.5"))) {
            entry->game_result = 0.5;
        } else if (strview_equals_strview(token, STATIC_STRVIEW("1.0"))) {
            entry->game_result = 1.0;
        } else {
            fprintf(
                stderr,
                "%s:%u:%u: Failed to parse game result\n",
                filename,
                (u32)lineno,
                (u32)space_index + 1
            );
            exit(EXIT_FAILURE);
        }

        line_view = strview_subview(line_view, 0, space_index);

        if (!board_try_init(&board, line_view, true, &stack)) {
            fprintf(stderr, "%s:%u: Failed to parse FEN\n", filename, (u32)lineno);
            exit(EXIT_FAILURE);
        }

        if (tuner_entry_init(entry, &board)) {
            ++tuner_dataset->size;
        } else {
            ++skipped;
        }
    }

    printf(
        "Loaded %u positions from '%s', skipped %u\n",
        (u32)(tuner_dataset->size - old_size),
        filename,
        (u32)skipped
    );
    fflush(stdout);
}

void tuner_dataset_start_session(TunerDataset *tuner_dataset, const TunerConfig *tuner_config) {
    AdamOptimizer adam;

    adam_init(&adam);
    f64 k = tuner_dataset_compute_optimal_k(tuner_dataset, tuner_config->lambda);
    tuner_dataset_compute_wdl_eval_mix(tuner_dataset, k, tuner_config->lambda);
}

f64 tuner_dataset_static_eval_mse(const TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda) {
    f64 total = 0.0;

#ifdef TUNE
#pragma omp parallel for schedule(static) reduction(+ : total)
#endif
    for (usize i = 0; i < tuner_dataset->size; ++i) {
        const TunerEntry *entry = &tuner_dataset->entries[i];
        const f64 lerp_result =
            entry->game_result * lambda + sigmoid(sigmoid_k, entry->search_score) * (1.0 - lambda);

        total += pow(lerp_result - sigmoid(sigmoid_k, entry->static_eval), 2.0);
    }

    return total / tuner_dataset->size;
}

static f64 golden_split(f64 a, f64 b) {
    const f64 invphi = 0.6180339887498949;
    return a + invphi * (b - a);
}

f64 tuner_dataset_compute_optimal_k(const TunerDataset *tuner_dataset, f64 lambda) {
    const f64 epsilon = 1e-10;
    f64 bound1 = 0.0;
    f64 bound2 = 1.0;
    f64 middle = golden_split(bound1, bound2);
    f64 best = tuner_dataset_static_eval_mse(tuner_dataset, middle, lambda);

    printf("Computing optimal K...\n");
    fflush(stdout);

    for (u32 i = 0; fabs(bound1 - bound2) > epsilon; ++i) {
        const f32 x = golden_split(bound1, middle);
        const f32 v = tuner_dataset_static_eval_mse(tuner_dataset, x, lambda);

        if (v < best) {
            best = v;
            bound2 = middle;
            middle = x;
        } else {
            bound1 = bound2;
            bound2 = x;
        }

        printf("Iteration %u, K %lf, MSE %lf\n", i, middle, best);
        fflush(stdout);
    }

    putchar('\n');
    return middle;
}

void tuner_dataset_compute_wdl_eval_mix(TunerDataset *tuner_dataset, f64 sigmoid_k, f64 lambda) {
    for (usize i = 0; i < tuner_dataset->size; ++i) {
        TunerEntry *entry = &tuner_dataset->entries[i];

        entry->game_result =
            entry->game_result * lambda + sigmoid(sigmoid_k, entry->search_score) * (1.0 - lambda);
    }
}

void adam_init(AdamOptimizer *adam) {
    tp_vector_reset(&adam->gradient);
    tp_vector_reset(&adam->momentum);
    tp_vector_reset(&adam->velocity);
}
