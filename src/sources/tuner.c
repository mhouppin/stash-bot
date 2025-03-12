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

#include "tuner.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef TUNE
#include <omp.h>
#endif

#include "psq_table.h"
#include "syncio.h"
#include "wmalloc.h"

f64 sigmoid(f64 k, f64 eval) {
    return 1.0 / (1.0 + exp(-eval * k));
}

f64 lerp(f64 lo, f64 hi, f64 rate) {
    return (hi * rate) + lo * (1.0 - rate);
}

void tuner_config_set_default_values(TunerConfig *tuner_config) {
    *tuner_config = (TunerConfig) {
        .threads = 1,
        .iterations = 10000,
        .display_every = 50,
        .batch_size = 16384,
        .lambda = 1.0,
        .learning_rate = 0.1,
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
    const f64 error = (entry->game_result - predict) * sqrt(fabs(entry->game_result - predict)) * predict * (1.0 - predict);
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
    TupleVector delta;
    TupleVector base;
    DisplaySequence disp_sequence;
    const f64 k = tuner_dataset_compute_optimal_k(tuner_dataset, tuner_config->lambda);

#ifdef TUNE
    omp_set_num_threads(tuner_config->threads);
#endif

    tuner_dataset_compute_wdl_eval_mix(tuner_dataset, k, tuner_config->lambda);
    adam_init(&adam);
    tp_vector_reset(&delta);
    init_disp_sequence_and_base_values(&disp_sequence, &base);

    for (usize iteration = 0; iteration < tuner_config->iterations; ++iteration) {
        adam_do_one_iteration(&adam, tuner_dataset, &delta, tuner_config, k);

        const f64 loss = tuner_dataset_adjusted_eval_mse(tuner_dataset, &delta, k);

        printf("Iteration [" FORMAT_LARGE_INT "], Loss [%.7lf]\n", (LargeInt)iteration, loss);

        if ((iteration + 1) % 50 == 0 || iteration + 1 == tuner_config->iterations) {
            disp_sequence_show(&disp_sequence, &base, &delta);
        }

        fflush(stdout);
    }
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

        total += pow(fabs(lerp_result - sigmoid(sigmoid_k, entry->static_eval)), 2.5);
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

f64 tuner_dataset_adjusted_eval_mse(
    const TunerDataset *restrict tuner_dataset,
    const TupleVector *restrict delta,
    f64 sigmoid_k
) {
    TupleSafetyEval safety_eval;
    f64 total = 0.0;

#ifdef TUNE
#pragma omp parallel for schedule(static) reduction(+ : total)
#endif
    for (usize i = 0; i < tuner_dataset->size; ++i) {
        const TunerEntry *entry = &tuner_dataset->entries[i];

        total +=
            pow(fabs(entry->game_result
                    - sigmoid(sigmoid_k, tuner_entry_adjusted_eval(entry, delta, &safety_eval))),
                2.5);
    }

    return total / tuner_dataset->size;
}

void tuner_dataset_compute_gradient(
    const TunerDataset *tuner_dataset,
    const TupleVector *restrict delta,
    TupleVector *restrict gradient,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k,
    usize batch_index
) {
    const usize batch_offset = batch_index * tuner_config->batch_size;
    const usize real_batch_size =
        usize_min(tuner_dataset->size - batch_offset, tuner_config->batch_size);

#ifdef TUNE
#pragma omp parallel shared(gradient)
#endif
    {
        TupleVector local;

        tp_vector_reset(&local);

#ifdef TUNE
#pragma omp for schedule(static)
#endif
        for (usize i = 0; i < real_batch_size; ++i) {
            tuner_entry_update_gradient(
                &tuner_dataset->entries[batch_offset + i],
                delta,
                gradient,
                sigmoid_k
            );
        }

#ifdef TUNE
#pragma omp critical
#endif
        for (usize i = 0; i < IDX_COUNT; ++i) {
            gradient->values[i][MIDGAME] += local.values[i][MIDGAME];
            gradient->values[i][ENDGAME] += local.values[i][ENDGAME];
        }
    }
}

void adam_init(AdamOptimizer *adam) {
    tp_vector_reset(&adam->gradient);
    tp_vector_reset(&adam->momentum);
    tp_vector_reset(&adam->velocity);
}

void adam_update_delta(
    AdamOptimizer *restrict adam,
    TupleVector *restrict delta,
    const TunerConfig *restrict tuner_config,
    f64 sigmoid_k
) {
    const f64 scale = sigmoid_k * 2.5 / tuner_config->batch_size;

    for (usize i = 0; i < IDX_COUNT; ++i) {
        for (Phase p = MIDGAME; p <= ENDGAME; ++p) {
            const f64 grad = adam->gradient.values[i][p] * scale;

            adam->momentum.values[i][p] =
                lerp(grad, adam->momentum.values[i][p], tuner_config->beta_1);
            adam->velocity.values[i][p] =
                lerp(grad * grad, adam->velocity.values[i][p], tuner_config->beta_2);
        }
    }

    for (usize i = 0; i < IDX_COUNT; ++i) {
        for (Phase p = MIDGAME; p <= ENDGAME; ++p) {
            delta->values[i][p] += adam->momentum.values[i][p] * tuner_config->learning_rate
                / sqrt(1e-8 + adam->velocity.values[i][p]);
        }
    }

    tp_vector_reset(&adam->gradient);
}

void adam_do_one_iteration(
    AdamOptimizer *restrict adam,
    const TunerDataset *restrict tuner_dataset,
    TupleVector *restrict delta,
    const TunerConfig *tuner_config,
    f64 sigmoid_k
) {
    const usize batch_count = usize_div_ceil(tuner_dataset->size, tuner_config->batch_size);

    for (usize batch_index = 0; batch_index < batch_count; ++batch_index) {
        tuner_dataset_compute_gradient(
            tuner_dataset,
            delta,
            &adam->gradient,
            tuner_config,
            sigmoid_k,
            batch_index
        );
        adam_update_delta(adam, delta, tuner_config, sigmoid_k);
    }
}

void disp_scorepair_show(
    const DisplayScorepair *disp_scorepair,
    const TupleVector *base,
    const TupleVector *delta
) {
    const u16 vecindex = disp_scorepair->index;
    const Score mg = round(base->values[vecindex][MIDGAME] + delta->values[vecindex][MIDGAME]);
    const Score eg = round(base->values[vecindex][ENDGAME] + delta->values[vecindex][ENDGAME]);

    fwrite_strview(stdout, STATIC_STRVIEW("const Scorepair "));
    fwrite_string(stdout, &disp_scorepair->name);

    for (usize i = disp_scorepair->name.size; i < disp_scorepair->name_alignment; ++i) {
        putchar(' ');
    }

    printf(
        " = SPAIR(%*d, %*d);\n",
        disp_scorepair->value_padding,
        mg,
        disp_scorepair->value_padding,
        eg
    );
}

void disp_sp_array_show(
    const DisplayScorepairArray *disp_sp_array,
    const TupleVector *base,
    const TupleVector *delta
) {
    fwrite_strview(stdout, STATIC_STRVIEW("const Scorepair "));
    fwrite_string(stdout, &disp_sp_array->name);
    printf("[%d] = {\n", disp_sp_array->array_size);

    for (u16 i = 0; i < disp_sp_array->array_size; ++i) {
        if (disp_sp_array->values_per_line == 0 || i % disp_sp_array->values_per_line == 0) {
            fwrite_strview(stdout, STATIC_STRVIEW("    "));
        }

        const bool last_value = (i + 1 == disp_sp_array->array_size);
        const bool last_line_value =
            (disp_sp_array->values_per_line == 0 || (i + 1) % disp_sp_array->values_per_line == 0);

        if (i < disp_sp_array->value_start || i >= disp_sp_array->value_end) {
            // Compute the bytes required for printing the values in the scorepair.
            // The calculation is as follows:
            // - 1 or 5 bytes for the prefix ("S" or "SPAIR");
            // - 5 bytes for the "(, )," part;
            // - (2 * value_padding) bytes for the mg/eg integers.
            // Don't right-pad the "0," if we're printing the rightmost term of a line.
            const u16 value_bytes = last_line_value || last_value
                ? 0
                : (disp_sp_array->long_prefix ? 5u : 1u) + 5
                    + u16_max(disp_sp_array->value_padding, 1) * 2;

            printf("%-*s", value_bytes, last_value ? "0" : "0,");
        } else {
            const u16 vecindex = disp_sp_array->index + i - disp_sp_array->value_start;
            const Score mg =
                round(base->values[vecindex][MIDGAME] + delta->values[vecindex][MIDGAME]);
            const Score eg =
                round(base->values[vecindex][ENDGAME] + delta->values[vecindex][ENDGAME]);

            printf(
                "%s(%*d, %*d)%s",
                disp_sp_array->long_prefix ? "SPAIR" : "S",
                disp_sp_array->value_padding,
                mg,
                disp_sp_array->value_padding,
                eg,
                last_value ? "" : ","
            );
        }

        putchar(!last_value && !last_line_value ? ' ' : '\n');
    }

    fwrite_strview(stdout, STATIC_STRVIEW("};\n\n"));
}

void disp_sequence_init(DisplaySequence *disp_sequence) {
    disp_sequence->blocks = NULL;
    disp_sequence->size = 0;
    disp_sequence->capacity = 0;
}

void disp_sequence_destroy(DisplaySequence *disp_sequence) {
    for (usize i = 0; i < disp_sequence->size; ++i) {
        DisplayBlock *block = &disp_sequence->blocks[i];

        switch (block->block_type) {
            case TypeScorepair: string_destroy(&block->data_union.scorepair.name); break;
            case TypeScorepairArray: string_destroy(&block->data_union.scorepair_array.name); break;
            case TypeRawString: string_destroy(&block->data_union.raw_string.str); break;
            case TypeNewline: break;
            case TypeCustom: break;
        }
    }

    free(disp_sequence->blocks);
    disp_sequence_init(disp_sequence);
}

void disp_sequence_maybe_extend_allocation(DisplaySequence *disp_sequence) {
    if (disp_sequence->size == disp_sequence->capacity) {
        disp_sequence->capacity += !disp_sequence->capacity ? 16 : disp_sequence->capacity / 4;
        disp_sequence->blocks =
            wrap_realloc(disp_sequence->blocks, sizeof(DisplayBlock) * disp_sequence->capacity);
    }
}

void disp_sequence_add_scorepair(
    DisplaySequence *disp_sequence,
    StringView name,
    u16 index,
    u16 name_alignment,
    u16 value_padding
) {
    disp_sequence_maybe_extend_allocation(disp_sequence);

    DisplayBlock *current = &disp_sequence->blocks[disp_sequence->size];
    DisplayScorepair *scorepair = &current->data_union.scorepair;

    current->block_type = TypeScorepair;
    string_init_from_strview(&scorepair->name, name);
    scorepair->index = index;
    scorepair->name_alignment = name_alignment;
    scorepair->value_padding = value_padding;
    ++disp_sequence->size;
}

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
) {
    disp_sequence_maybe_extend_allocation(disp_sequence);

    DisplayBlock *current = &disp_sequence->blocks[disp_sequence->size];
    DisplayScorepairArray *sp_array = &current->data_union.scorepair_array;

    current->block_type = TypeScorepairArray;
    string_init_from_strview(&sp_array->name, name);
    sp_array->index = index;
    sp_array->array_size = array_size;
    sp_array->value_start = value_start;
    sp_array->value_end = value_end;
    sp_array->value_padding = value_padding;
    sp_array->values_per_line = values_per_line;
    sp_array->long_prefix = long_prefix;
    ++disp_sequence->size;
}

void disp_sequence_add_raw_string(DisplaySequence *disp_sequence, StringView str) {
    disp_sequence_maybe_extend_allocation(disp_sequence);

    DisplayBlock *current = &disp_sequence->blocks[disp_sequence->size];
    DisplayRawString *raw_string = &current->data_union.raw_string;

    current->block_type = TypeRawString;
    string_init_from_strview(&raw_string->str, str);
    ++disp_sequence->size;
}

void disp_sequence_add_newline(DisplaySequence *disp_sequence) {
    disp_sequence_maybe_extend_allocation(disp_sequence);

    DisplayBlock *current = &disp_sequence->blocks[disp_sequence->size];

    current->block_type = TypeNewline;
    ++disp_sequence->size;
}

void disp_sequence_add_custom(
    DisplaySequence *disp_sequence,
    void (*custom_display)(const TupleVector *, const TupleVector *)
) {
    disp_sequence_maybe_extend_allocation(disp_sequence);

    DisplayBlock *current = &disp_sequence->blocks[disp_sequence->size];
    DisplayCustomData *custom = &current->data_union.custom;

    current->block_type = TypeCustom;
    custom->custom_display = custom_display;
    ++disp_sequence->size;
}

void disp_sequence_show(
    const DisplaySequence *restrict disp_sequence,
    const TupleVector *restrict base,
    const TupleVector *restrict delta
) {
    for (usize i = 0; i < disp_sequence->size; ++i) {
        const DisplayBlock *block = &disp_sequence->blocks[i];

        switch (block->block_type) {
            case TypeScorepair:
                disp_scorepair_show(&block->data_union.scorepair, base, delta);
                break;

            case TypeScorepairArray:
                disp_sp_array_show(&block->data_union.scorepair_array, base, delta);
                break;

            case TypeRawString: fwrite_string(stdout, &block->data_union.raw_string.str); break;

            case TypeNewline: putchar('\n'); break;

            case TypeCustom: block->data_union.custom.custom_display(base, delta); break;
        }
    }
}

void base_values_set_scorepair(TupleVector *base, Scorepair value, u16 index) {
    base->values[index][MIDGAME] = scorepair_midgame(value);
    base->values[index][ENDGAME] = scorepair_endgame(value);
}

void base_values_set_sp_array(
    TupleVector *restrict base,
    const Scorepair *restrict sp_array,
    u16 index,
    u16 value_start,
    u16 value_end
) {
    for (u16 i = value_start; i < value_end; ++i) {
        const u16 vecindex = index + i - value_start;

        base->values[vecindex][MIDGAME] = scorepair_midgame(sp_array[i]);
        base->values[vecindex][ENDGAME] = scorepair_endgame(sp_array[i]);
    }
}

// Custom function for displaying piece values, as they are stored in an enum
static void display_piece_values(const TupleVector *base, const TupleVector *delta) {
    static const char *PieceNames[5] = {"PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN"};

    fwrite_strview(
        stdout,
        STATIC_STRVIEW("// Enum for all pieces' midgame, endgame and SEE scores\nenum {\n")
    );

    for (Phase p = MIDGAME; p <= ENDGAME; ++p) {
        for (Piecetype pt = PAWN; pt <= QUEEN; ++pt) {
            const u16 vecindex = IDX_PIECE + pt - PAWN;

            printf(
                "    %s_%cG_SCORE = %.lf,\n",
                PieceNames[pt - PAWN],
                p == MIDGAME ? 'M' : 'E',
                base->values[vecindex][p] + delta->values[vecindex][p]
            );
        }

        putchar('\n');
    }

    printf("    PAWN_SEE_SCORE = %d,\n", PAWN_SEE_SCORE);
    printf("    KNIGHT_SEE_SCORE = %d,\n", KNIGHT_SEE_SCORE);
    printf("    BISHOP_SEE_SCORE = %d,\n", BISHOP_SEE_SCORE);
    printf("    ROOK_SEE_SCORE = %d,\n", ROOK_SEE_SCORE);
    printf("    QUEEN_SEE_SCORE = %d,\n", QUEEN_SEE_SCORE);
    fwrite_strview(stdout, STATIC_STRVIEW("};\n"));
}

void init_disp_sequence_and_base_values(
    DisplaySequence *restrict disp_sequence,
    TupleVector *restrict base
) {
    disp_sequence_init(disp_sequence);
    tp_vector_reset(base);

    // psq_table.h values
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("\n[psq_table.h]\n\n"));
    disp_sequence_add_custom(disp_sequence, display_piece_values);

    // Record the piece values manually.
    for (Phase p = MIDGAME; p <= ENDGAME; ++p) {
        for (Piecetype pt = PAWN; pt <= QUEEN; ++pt) {
            base->values[IDX_PIECE + pt - PAWN][p] = PieceScores[p][create_piece(WHITE, pt)];
        }
    }

    // psq_table.c values
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("\n[psq_table.c]\n\n"));

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Square-based Pawn scoring for evaluation\n")
    );
    TUNE_ADD_SP_ARRAY(PawnSQT, IDX_PSQT, 48, 0, 48, 3, 8, false);

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Square-based piece scoring for evaluation, using a file symmetry\n")
    );
    TUNE_ADD_SP_ARRAY(KnightSQT, IDX_PSQT + 48, 32, 0, 32, 4, 4, false);
    TUNE_ADD_SP_ARRAY(BishopSQT, IDX_PSQT + 80, 32, 0, 32, 4, 4, false);
    TUNE_ADD_SP_ARRAY(RookSQT, IDX_PSQT + 112, 32, 0, 32, 4, 4, false);
    TUNE_ADD_SP_ARRAY(QueenSQT, IDX_PSQT + 144, 32, 0, 32, 4, 4, false);
    TUNE_ADD_SP_ARRAY(KingSQT, IDX_PSQT + 176, 32, 0, 32, 4, 4, false);

    // evaluate.c values
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("\n[evaluate.c]\n\n"));

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Special eval terms\n"));
    TUNE_ADD_SCOREPAIR(Initiative, IDX_INITIATIVE, 0, 2);
    disp_sequence_add_newline(disp_sequence);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Knight eval terms\n"));
    TUNE_ADD_SCOREPAIR(KnightShielded, IDX_KNIGHT_SHIELDED, 14, 3);
    TUNE_ADD_SCOREPAIR(KnightOutpost, IDX_KNIGHT_OUTPOST, 14, 3);
    disp_sequence_add_newline(disp_sequence);

    TUNE_ADD_SP_ARRAY(ClosedPosKnight, IDX_KNIGHT_CLOSED_POS, 5, 0, 5, 3, 4, true);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Bishop eval terms\n"));
    TUNE_ADD_SCOREPAIR(BishopPairBonus, IDX_BISHOP_PAIR, 18, 3);
    TUNE_ADD_SCOREPAIR(BishopShielded, IDX_BISHOP_SHIELDED, 18, 3);
    TUNE_ADD_SCOREPAIR(BishopOutpost, IDX_BISHOP_OUTPOST, 18, 3);
    TUNE_ADD_SCOREPAIR(BishopLongDiagonal, IDX_BISHOP_LONG_DIAG, 18, 3);
    disp_sequence_add_newline(disp_sequence);

    TUNE_ADD_SP_ARRAY(BishopPawnsSameColor, IDX_BISHOP_PAWNS_COLOR, 7, 0, 7, 3, 4, true);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Rook eval terms\n"));
    TUNE_ADD_SCOREPAIR(RookOnSemiOpenFile, IDX_ROOK_SEMIOPEN, 18, 3);
    TUNE_ADD_SCOREPAIR(RookOnOpenFile, IDX_ROOK_OPEN, 18, 3);
    TUNE_ADD_SCOREPAIR(RookOnBlockedFile, IDX_ROOK_BLOCKED, 18, 3);
    TUNE_ADD_SCOREPAIR(RookXrayQueen, IDX_ROOK_XRAY_QUEEN, 18, 3);
    TUNE_ADD_SCOREPAIR(RookTrapped, IDX_ROOK_TRAPPED, 18, 3);
    TUNE_ADD_SCOREPAIR(RookBuried, IDX_ROOK_BURIED, 18, 3);
    disp_sequence_add_newline(disp_sequence);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Mobility eval terms\n"));
    TUNE_ADD_SP_ARRAY(KnightMobility, IDX_MOBILITY_KNIGHT, 9, 0, 9, 4, 4, true);
    TUNE_ADD_SP_ARRAY(BishopMobility, IDX_MOBILITY_BISHOP, 14, 0, 14, 4, 4, true);
    TUNE_ADD_SP_ARRAY(RookMobility, IDX_MOBILITY_ROOK, 15, 0, 15, 4, 4, true);
    TUNE_ADD_SP_ARRAY(QueenMobility, IDX_MOBILITY_QUEEN, 28, 0, 28, 4, 4, true);

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// King Safety linear eval terms\n")
    );
    TUNE_ADD_SCOREPAIR(FarKnight, IDX_FAR_KNIGHT, 9, 3);
    TUNE_ADD_SCOREPAIR(FarBishop, IDX_FAR_BISHOP, 9, 3);
    TUNE_ADD_SCOREPAIR(FarRook, IDX_FAR_ROOK, 9, 3);
    TUNE_ADD_SCOREPAIR(FarQueen, IDX_FAR_QUEEN, 9, 3);
    disp_sequence_add_newline(disp_sequence);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// King Safety eval terms\n"));
    TUNE_ADD_SCOREPAIR(KnightWeight, IDX_KS_KNIGHT, 15, 4);
    TUNE_ADD_SCOREPAIR(BishopWeight, IDX_KS_BISHOP, 15, 4);
    TUNE_ADD_SCOREPAIR(RookWeight, IDX_KS_ROOK, 15, 4);
    TUNE_ADD_SCOREPAIR(QueenWeight, IDX_KS_QUEEN, 15, 4);
    TUNE_ADD_SCOREPAIR(AttackWeight, IDX_KS_ATTACK, 15, 4);
    TUNE_ADD_SCOREPAIR(WeakKingZone, IDX_KS_WEAK_Z, 15, 4);
    TUNE_ADD_SCOREPAIR(SafeKnightCheck, IDX_KS_CHECK_N, 15, 4);
    TUNE_ADD_SCOREPAIR(SafeBishopCheck, IDX_KS_CHECK_B, 15, 4);
    TUNE_ADD_SCOREPAIR(SafeRookCheck, IDX_KS_CHECK_R, 15, 4);
    TUNE_ADD_SCOREPAIR(SafeQueenCheck, IDX_KS_CHECK_Q, 15, 4);
    TUNE_ADD_SCOREPAIR(UnsafeCheck, IDX_KS_UNSAFE_CHECK, 15, 4);
    TUNE_ADD_SCOREPAIR(QueenlessAttack, IDX_KS_QUEENLESS, 15, 4);
    TUNE_ADD_SCOREPAIR(SafetyOffset, IDX_KS_OFFSET, 15, 4);
    disp_sequence_add_newline(disp_sequence);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Storm/Shelter indexes:\n"));
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// 0-7 - Side\n"));
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// 9-15 - Front\n"));
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// 16-23 - Center\n"));
    TUNE_ADD_SP_ARRAY(KingStorm, IDX_KS_STORM, 24, 0, 24, 4, 4, true);
    TUNE_ADD_SP_ARRAY(KingShelter, IDX_KS_SHELTER, 24, 0, 24, 4, 4, true);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Threat eval terms\n"));
    TUNE_ADD_SCOREPAIR(PawnAttacksMinor, IDX_PAWN_ATK_MINOR, 17, 3);
    TUNE_ADD_SCOREPAIR(PawnAttacksRook, IDX_PAWN_ATK_ROOK, 17, 3);
    TUNE_ADD_SCOREPAIR(PawnAttacksQueen, IDX_PAWN_ATK_QUEEN, 17, 3);
    TUNE_ADD_SCOREPAIR(MinorAttacksRook, IDX_MINOR_ATK_ROOK, 17, 3);
    TUNE_ADD_SCOREPAIR(MinorAttacksQueen, IDX_MINOR_ATK_QUEEN, 17, 3);
    TUNE_ADD_SCOREPAIR(RookAttacksQueen, IDX_ROOK_ATK_QUEEN, 17, 3);
    TUNE_ADD_SCOREPAIR(HangingPawn, IDX_HANGING_PAWN, 17, 3);
    disp_sequence_add_newline(disp_sequence);

    // kp_eval.c values
    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("\n[kp_eval.c]\n\n"));

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Miscellanous bonus for Pawn structures\n")
    );
    TUNE_ADD_SCOREPAIR(BackwardPenalty, IDX_BACKWARD, 16, 3);
    TUNE_ADD_SCOREPAIR(StragglerPenalty, IDX_STRAGGLER, 16, 3);
    TUNE_ADD_SCOREPAIR(DoubledPenalty, IDX_DOUBLED, 16, 3);
    TUNE_ADD_SCOREPAIR(IsolatedPenalty, IDX_ISOLATED, 16, 3);
    disp_sequence_add_newline(disp_sequence);

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Rank-based bonus for passed Pawns\n")
    );
    TUNE_ADD_SP_ARRAY(PassedBonus, IDX_PASSER, 8, 1, 7, 3, 1, true);

    disp_sequence_add_raw_string(disp_sequence, STATIC_STRVIEW("// Passed Pawn eval terms\n"));
    TUNE_ADD_SP_ARRAY(PassedOurKingDistance, IDX_PASSED_OUR_KING_DIST, 24, 0, 24, 4, 3, true);
    TUNE_ADD_SP_ARRAY(PassedTheirKingDistance, IDX_PASSED_THEIR_KING_DIST, 24, 0, 24, 4, 3, true);

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Rank-based bonus for phalanx structures\n")
    );
    TUNE_ADD_SP_ARRAY(PhalanxBonus, IDX_PHALANX, 8, 1, 7, 3, 1, true);

    disp_sequence_add_raw_string(
        disp_sequence,
        STATIC_STRVIEW("// Rank-based bonus for defenders\n")
    );
    TUNE_ADD_SP_ARRAY(DefenderBonus, IDX_DEFENDER, 8, 1, 6, 3, 1, true);
}
