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

#include "option.h"

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include "syncio.h"
#include "wmalloc.h"

void option_button_show(StringView name, const OptionParams *params) {
    __attribute__((unused)) const OptButtonParams *button_params = &params->button;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fwrite_strview(stdout, STATIC_STRVIEW(" type button\n"));
    sync_unlock_stdout();
}

void option_spin_integer_show(StringView name, const OptionParams *params) {
    const OptSpinIntegerParams *spin_integer_params = &params->spin_integer;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        " type spin default %" PRIi64 " min %" PRIi64 " max %" PRIi64 "\n",
        spin_integer_params->default_value,
        spin_integer_params->min_value,
        spin_integer_params->max_value
    );
    sync_unlock_stdout();
}

void option_spin_float_show(StringView name, const OptionParams *params) {
    const OptSpinFloatParams *spin_float_params = &params->spin_float;
    const i64 default_qvalue =
        (i64)round(spin_float_params->default_value * spin_float_params->resolution);
    const i64 min_qvalue = (i64)round(spin_float_params->min_value * spin_float_params->resolution);
    const i64 max_qvalue = (i64)round(spin_float_params->max_value * spin_float_params->resolution);

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        " type spin default %" PRIi64 " min %" PRIi64 " max %" PRIi64 "\n",
        default_qvalue,
        min_qvalue,
        max_qvalue
    );
    sync_unlock_stdout();
}

void option_check_show(StringView name, const OptionParams *params) {
    const OptCheckParams *check_params = &params->check;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fprintf(stdout, " type check default %s\n", check_params->default_value ? "true" : "false");
    sync_unlock_stdout();
}

void option_string_show(StringView name, const OptionParams *params) {
    const OptStringParams *string_params = &params->string;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fwrite_strview(stdout, STATIC_STRVIEW(" type string default "));
    fwrite_string(stdout, &string_params->default_value);
    fputc('\n', stdout);
    sync_unlock_stdout();
}

void option_combo_show(StringView name, const OptionParams *params) {
    const OptComboParams *combo_params = &params->combo;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fwrite_strview(stdout, STATIC_STRVIEW(" type combo default "));
    fwrite_string(stdout, &combo_params->default_value);

    for (usize i = 0; i < combo_params->allowed_count; ++i) {
        fwrite_strview(stdout, STATIC_STRVIEW(" var "));
        fwrite_string(stdout, &combo_params->allowed_values[i]);
    }

    fputc('\n', stdout);
    sync_unlock_stdout();
}

void option_score_show(StringView name, const OptionParams *params) {
    const OptScoreParams *score_params = &params->score;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        " type spin default %" PRIi16 " min %" PRIi16 " max %" PRIi16 "\n",
        score_params->default_value,
        score_params->min_value,
        score_params->max_value
    );
    sync_unlock_stdout();
}

void option_half_scorepair_show(StringView name, const OptionParams *params) {
    const OptHalfScorepairParams *half_scorepair_params = &params->half_scorepair;

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("option name "));
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        " type spin default %" PRIi16 " min %" PRIi16 " max %" PRIi16 "\n",
        half_scorepair_params->default_value,
        half_scorepair_params->min_value,
        half_scorepair_params->max_value
    );
    sync_unlock_stdout();
}

void option_default_show_tune(
    __attribute__((unused)) StringView name,
    __attribute__((unused)) const OptionParams *params
) {
}

void option_spin_integer_show_tune(StringView name, const OptionParams *params) {
    const OptSpinIntegerParams *spin_integer_params = &params->spin_integer;

    if (!spin_integer_params->is_tunable) {
        return;
    }

    const i64 step =
        i64_max(1, (spin_integer_params->max_value - spin_integer_params->min_value) / 20);

    sync_lock_stdout();
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        ", int, %" PRIi64 ", %" PRIi64 ", %" PRIi64 ", %" PRIi64 ", 0.002",
        spin_integer_params->default_value,
        spin_integer_params->min_value,
        spin_integer_params->max_value,
        step
    );
    sync_unlock_stdout();
}

void option_spin_float_show_tune(StringView name, const OptionParams *params) {
    const OptSpinFloatParams *spin_float_params = &params->spin_float;

    if (!spin_float_params->is_tunable) {
        return;
    }

    const i64 default_qvalue =
        (i64)round(spin_float_params->default_value * spin_float_params->resolution);
    const i64 min_qvalue = (i64)round(spin_float_params->min_value * spin_float_params->resolution);
    const i64 max_qvalue = (i64)round(spin_float_params->max_value * spin_float_params->resolution);
    const i64 step = i64_max(1, (max_qvalue - min_qvalue) / 20);

    sync_lock_stdout();
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        ", int, %" PRIi64 ", %" PRIi64 ", %" PRIi64 ", %" PRIi64 ", 0.002",
        default_qvalue,
        min_qvalue,
        max_qvalue,
        step
    );
    sync_unlock_stdout();
}

void option_score_show_tune(StringView name, const OptionParams *params) {
    const OptScoreParams *score_params = &params->score;

    if (!score_params->is_tunable) {
        return;
    }

    const i64 step = i64_max(1, ((i64)score_params->max_value - (i64)score_params->min_value) / 20);

    sync_lock_stdout();
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        ", int, %" PRIi16 ", %" PRIi16 ", %" PRIi16 ", %" PRIi64 ", 0.002",
        score_params->default_value,
        score_params->min_value,
        score_params->max_value,
        step
    );
    sync_unlock_stdout();
}

void option_half_scorepair_show_tune(StringView name, const OptionParams *params) {
    const OptHalfScorepairParams *half_scorepair_params = &params->half_scorepair;

    if (!half_scorepair_params->is_tunable) {
        return;
    }

    const i64 step = i64_max(
        1,
        ((i64)half_scorepair_params->max_value - (i64)half_scorepair_params->min_value) / 20
    );

    sync_lock_stdout();
    fwrite_strview(stdout, name);
    fprintf(
        stdout,
        ", int, %" PRIi16 ", %" PRIi16 ", %" PRIi16 ", %" PRIi64 ", 0.002",
        half_scorepair_params->default_value,
        half_scorepair_params->min_value,
        half_scorepair_params->max_value,
        step
    );
    sync_unlock_stdout();
}

bool option_button_try_set(__attribute__((unused)) OptionParams *params, StringView value) {
    if (value.size != 0) {
        sync_lock_stdout();
        fwrite_strview(
            stdout,
            STATIC_STRVIEW("info string error: expected no value for button option, got '")
        );
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("'\n"));
        sync_unlock_stdout();
        return false;
    }

    return true;
}

bool option_spin_integer_try_set(OptionParams *params, StringView value) {
    OptSpinIntegerParams *spin_integer_params = &params->spin_integer;
    i64 v;

    if (!strview_parse_i64(value, &v)) {
        sync_lock_stdout();
        fwrite_strview(stdout, STATIC_STRVIEW("info string error: failed to parse '"));
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("' as an integer\n"));
        sync_unlock_stdout();
        return false;
    }

    if (v < spin_integer_params->min_value || v > spin_integer_params->max_value) {
        fprintf(
            stdout,
            "info string error: %" PRIi64 " is outside the expected range (%" PRIi64
            " <= x <= %" PRIi64 ")\n",
            v,
            spin_integer_params->min_value,
            spin_integer_params->max_value
        );
        return false;
    }

    *spin_integer_params->current_value = v;
    return true;
}

bool option_spin_float_try_set(OptionParams *params, StringView value) {
    OptSpinFloatParams *spin_float_params = &params->spin_float;
    i64 v;

    if (!strview_parse_i64(value, &v)) {
        sync_lock_stdout();
        fwrite_strview(stdout, STATIC_STRVIEW("info string error: failed to parse '"));
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("' as an integer\n"));
        sync_unlock_stdout();
        return false;
    }

    const i64 min_qvalue = (i64)round(spin_float_params->min_value * spin_float_params->resolution);
    const i64 max_qvalue = (i64)round(spin_float_params->max_value * spin_float_params->resolution);

    if (v < min_qvalue || v > max_qvalue) {
        fprintf(
            stdout,
            "info string error: %" PRIi64 " is outside the expected range (%" PRIi64
            " <= x <= %" PRIi64 ")\n",
            v,
            min_qvalue,
            max_qvalue
        );
        return false;
    }

    *spin_float_params->current_value = fmax(
        fmin((f64)v / spin_float_params->resolution, spin_float_params->max_value),
        spin_float_params->min_value
    );
    return true;
}

bool option_check_try_set(OptionParams *params, StringView value) {
    OptCheckParams *check_params = &params->check;

    if (strview_equals_strview_nocase(value, strview_from_cstr("true"))) {
        *check_params->current_value = true;
    } else if (strview_equals_strview_nocase(value, strview_from_cstr("false"))) {
        *check_params->current_value = false;
    } else {
        sync_lock_stdout();
        fwrite_strview(stdout, STATIC_STRVIEW("info string error: failed to parse '"));
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("' as a boolean\n"));
        sync_unlock_stdout();
        return false;
    }

    return true;
}

bool option_string_try_set(OptionParams *params, StringView value) {
    OptStringParams *string_params = &params->string;

    string_clear(string_params->current_value);
    string_push_back_strview(string_params->current_value, value);
    return true;
}

bool option_combo_try_set(OptionParams *params, StringView value) {
    OptComboParams *combo_params = &params->combo;

    for (usize i = 0; i < combo_params->allowed_count; ++i) {
        StringView combo_value = strview_from_string(&combo_params->allowed_values[i]);

        if (strview_equals_strview_nocase(value, combo_value)) {
            string_clear(combo_params->current_value);
            string_push_back_strview(combo_params->current_value, combo_value);
            return true;
        }
    }

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("info string error: the value '"));
    fwrite_strview(stdout, value);
    fwrite_strview(
        stdout,
        STATIC_STRVIEW("' does not match any of the allowed values in the combo list\n")
    );
    sync_unlock_stdout();
    return false;
}

bool option_score_try_set(OptionParams *params, StringView value) {
    OptScoreParams *score_params = &params->score;
    i64 v;

    if (!strview_parse_i64(value, &v)) {
        sync_lock_stdout();
        fwrite_strview(stdout, STATIC_STRVIEW("info string error: failed to parse '"));
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("' as an integer\n"));
        sync_unlock_stdout();
        return false;
    }

    if (v < (i64)score_params->min_value || v > (i64)score_params->max_value) {
        fprintf(
            stdout,
            "info string error: %" PRIi64 " is outside the expected range (%" PRIi16
            " <= x <= %" PRIi16 ")\n",
            v,
            score_params->min_value,
            score_params->max_value
        );
        return false;
    }

    *score_params->current_value = v;
    return true;
}

bool option_half_scorepair_try_set(OptionParams *params, StringView value) {
    OptHalfScorepairParams *half_scorepair_params = &params->half_scorepair;
    i64 v;

    if (!strview_parse_i64(value, &v)) {
        sync_lock_stdout();
        fwrite_strview(stdout, STATIC_STRVIEW("info string error: failed to parse '"));
        fwrite_strview(stdout, value);
        fwrite_strview(stdout, STATIC_STRVIEW("' as an integer\n"));
        sync_unlock_stdout();
        return false;
    }

    if (v < (i64)half_scorepair_params->min_value || v > (i64)half_scorepair_params->max_value) {
        fprintf(
            stdout,
            "info string error: %" PRIi64 " is outside the expected range (%" PRIi16
            " <= x <= %" PRIi16 ")\n",
            v,
            half_scorepair_params->min_value,
            half_scorepair_params->max_value
        );
        return false;
    }

    Score midgame, endgame;

    if (half_scorepair_params->phase == MIDGAME) {
        midgame = v;
        endgame = scorepair_endgame(*half_scorepair_params->current_value);
    } else {
        midgame = scorepair_midgame(*half_scorepair_params->current_value);
        endgame = v;
    }

    *half_scorepair_params->current_value = create_scorepair(midgame, endgame);
    return true;
}

void option_default_dtor(__attribute__((unused)) OptionParams *params) {
}

void option_string_dtor(OptionParams *params) {
    OptStringParams *string_params = &params->string;

    // Note: we cannot destroy current_value here, as it don't have its exclusive ownership.
    string_destroy(&string_params->default_value);
}

void option_combo_dtor(OptionParams *params) {
    OptComboParams *combo_params = &params->combo;

    // Note: we cannot destroy current_value here, as it don't have its exclusive ownership.
    string_destroy(&combo_params->default_value);

    for (usize i = 0; i < combo_params->allowed_count; ++i) {
        string_destroy(&combo_params->allowed_values[i]);
    }

    free(combo_params->allowed_values);
}

// clang-format off
const OptionVtable OptionTypeVtables[OPTION_TYPE_COUNT] = {
    {
        option_button_show,
        option_default_show_tune,
        option_button_try_set,
        option_default_dtor,
    },
    {
        option_spin_integer_show,
        option_spin_integer_show_tune,
        option_spin_integer_try_set,
        option_default_dtor
    },
    {
        option_spin_float_show,
        option_spin_float_show_tune,
        option_spin_float_try_set,
        option_default_dtor
    },
    {
        option_check_show,
        option_default_show_tune,
        option_check_try_set,
        option_default_dtor
    },
    {
        option_string_show,
        option_default_show_tune,
        option_string_try_set,
        option_string_dtor
    },
    {
        option_combo_show,
        option_default_show_tune,
        option_combo_try_set,
        option_combo_dtor
    },
    {
        option_score_show,
        option_score_show_tune,
        option_score_try_set,
        option_default_dtor
    },
    {
        option_half_scorepair_show,
        option_half_scorepair_show_tune,
        option_half_scorepair_try_set,
        option_default_dtor
    }
};
// clang-format on

// Helper function for initializing struct fields common to all option types.
static void option_init_common(
    Option *option,
    StringView name,
    OptionType option_type,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    string_init_from_strview(&option->option_name, name);
    option->option_type = option_type;
    option->option_vtable = &OptionTypeVtables[option_type];
    option->setoption_callback = setoption_callback;
    option->callback_data = callback_data;
}

static void optlist_maybe_extend_capacity(OptionList *optlist) {
    if (optlist->size < optlist->capacity) {
        return;
    }

    usize new_capacity = optlist->capacity * 3 / 2 + 1;

    optlist->options = wrap_realloc(optlist->options, new_capacity * sizeof(Option));
    optlist->capacity = new_capacity;
}

void optlist_init(OptionList *optlist) {
    optlist->options = NULL;
    optlist->size = 0;
    optlist->capacity = 0;
}

void optlist_destroy(OptionList *optlist) {
    for (usize i = 0; i < optlist->size; ++i) {
        Option *cur_option = &optlist->options[i];

        string_destroy(&cur_option->option_name);
        cur_option->option_vtable->option_dtor(&cur_option->option_params);
    }

    free(optlist->options);
    optlist_init(optlist);
}

void optlist_show_options(const OptionList *optlist) {
    for (usize i = 0; i < optlist->size; ++i) {
        const Option *cur_option = &optlist->options[i];

        cur_option->option_vtable->option_show(
            strview_from_string(&cur_option->option_name),
            &cur_option->option_params
        );
    }
}

void optlist_show_tunable_options(const OptionList *optlist) {
    for (usize i = 0; i < optlist->size; ++i) {
        const Option *cur_option = &optlist->options[i];

        cur_option->option_vtable->option_show_tune(
            strview_from_string(&cur_option->option_name),
            &cur_option->option_params
        );
    }
}

void optlist_set_option(OptionList *optlist, StringView name, StringView value) {
    // TODO: this is far from optimal, and has a runtime of O(n). What we want to do later is keep a
    // hashmap of all options so that finding the correct option has a runtime of O(log n), to avoid
    // large initialization delays when running SPSA tests with a lot of tweakable parameters.
    for (usize i = 0; i < optlist->size; ++i) {
        Option *cur_option = &optlist->options[i];

        if (strview_equals_strview_nocase(strview_from_string(&cur_option->option_name), name)) {
            if (!cur_option->option_vtable->option_try_set(&cur_option->option_params, value)) {
                sync_lock_stdout();
                fwrite_strview(stdout, STATIC_STRVIEW("info string error: Unable to set option '"));
                fwrite_string(stdout, &cur_option->option_name);
                fwrite_strview(stdout, STATIC_STRVIEW("'\n"));
                sync_unlock_stdout();
                return;
            }

            info_debug(
                "info string Setting option '%.*s' to '%.*s'\n",
                (int)cur_option->option_name.size,
                (const char *)cur_option->option_name.data,
                (int)value.size,
                (const char *)value.data
            );

            if (cur_option->setoption_callback != NULL) {
                cur_option->setoption_callback(
                    &cur_option->option_params,
                    cur_option->callback_data
                );
            }

            return;
        }
    }

    sync_lock_stdout();
    fwrite_strview(stdout, STATIC_STRVIEW("info string error: option '"));
    fwrite_strview(stdout, name);
    fwrite_strview(stdout, STATIC_STRVIEW("' does not exist\n"));
    sync_unlock_stdout();
}

void optlist_add_button(
    OptionList *optlist,
    StringView name,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    new_option->option_params = (OptionParams) {.button = {}};
    option_init_common(new_option, name, OptionButton, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_spin_integer(
    OptionList *optlist,
    StringView name,
    i64 *value,
    i64 minval,
    i64 maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    // clang-format off
    new_option->option_params = (OptionParams) {
        .spin_integer = (OptSpinIntegerParams) {
           .current_value = value,
           .default_value = *value,
           .min_value = minval,
           .max_value = maxval,
           .is_tunable = is_tunable,
       }
    };
    // clang-format on
    option_init_common(new_option, name, OptionSpinInteger, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_spin_float(
    OptionList *optlist,
    StringView name,
    f64 *value,
    f64 minval,
    f64 maxval,
    i64 resolution,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    // clang-format off
    new_option->option_params = (OptionParams) {
        .spin_float = (OptSpinFloatParams) {
            .current_value = value,
            .default_value = *value,
            .min_value = minval,
            .max_value = maxval,
            .resolution = resolution,
            .is_tunable = is_tunable,
        }
    };
    // clang-format on
    option_init_common(new_option, name, OptionSpinFloat, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_check(
    OptionList *optlist,
    StringView name,
    bool *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    // clang-format off
    new_option->option_params = (OptionParams) {
        .check = (OptCheckParams) {
            .current_value = value,
            .default_value = *value,
        }
    };
    // clang-format on
    option_init_common(new_option, name, OptionCheck, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_string(
    OptionList *optlist,
    StringView name,
    String *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    new_option->option_params.string.current_value = value;
    string_init_from_strview(
        &new_option->option_params.string.default_value,
        strview_from_string(value)
    );
    option_init_common(new_option, name, OptionString, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_combo(
    OptionList *optlist,
    StringView name,
    String *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data,
    usize allowed_count,
    ...
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];
    OptComboParams *params = &new_option->option_params.combo;
    va_list ap;

    params->current_value = value;
    string_init_from_strview(&params->default_value, strview_from_string(value));
    params->allowed_count = allowed_count;
    params->allowed_values = wrap_malloc(allowed_count * sizeof(String));
    va_start(ap, allowed_count);

    for (usize i = 0; i < allowed_count; ++i) {
        string_init_from_strview(&params->allowed_values[i], va_arg(ap, StringView));
    }

    va_end(ap);
    option_init_common(new_option, name, OptionCombo, setoption_callback, callback_data);
    optlist->size++;
}

void optlist_add_score(
    OptionList *optlist,
    StringView name,
    Score *value,
    Score minval,
    Score maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];

    // clang-format off
    new_option->option_params = (OptionParams) {
        .score = (OptScoreParams) {
            .current_value = value,
            .default_value = *value,
            .min_value = minval,
            .max_value = maxval,
            .is_tunable = is_tunable,
        }
    };
    // clang-format on
    option_init_common(new_option, name, OptionScore, setoption_callback, callback_data);
    optlist->size++;
}

static void optlist_add_half_scorepair(
    OptionList *optlist,
    StringView name,
    Scorepair *value,
    Score minval,
    Score maxval,
    bool is_tunable,
    Phase phase,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_maybe_extend_capacity(optlist);

    Option *new_option = &optlist->options[optlist->size];
    String ext_name;
    const Score defval = (phase == MIDGAME) ? scorepair_midgame(*value) : scorepair_endgame(*value);

    // clang-format off
    new_option->option_params = (OptionParams) {
        .half_scorepair = (OptHalfScorepairParams) {
            .current_value = value,
            .default_value = defval,
            .min_value = minval,
            .max_value = maxval,
            .is_tunable = is_tunable,
        }
    };
    // clang-format on
    string_init_from_strview(&ext_name, name);
    string_push_back_strview(&ext_name, strview_from_cstr(phase == MIDGAME ? "Mg" : "Eg"));
    option_init_common(
        new_option,
        strview_from_string(&ext_name),
        OptionHalfScorepair,
        setoption_callback,
        callback_data
    );
    string_destroy(&ext_name);
    optlist->size++;
}

void optlist_add_scorepair(
    OptionList *optlist,
    StringView name,
    Scorepair *value,
    Score minval,
    Score maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
) {
    optlist_add_half_scorepair(
        optlist,
        name,
        value,
        minval,
        maxval,
        is_tunable,
        MIDGAME,
        setoption_callback,
        callback_data
    );
    optlist_add_half_scorepair(
        optlist,
        name,
        value,
        minval,
        maxval,
        is_tunable,
        ENDGAME,
        setoption_callback,
        callback_data
    );
}
