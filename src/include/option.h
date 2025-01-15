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

#ifndef OPTION_H
#define OPTION_H

#include "chess_types.h"
#include "core.h"
#include "strmanip.h"

// Enum for supported option types
typedef enum _OptionType {
    OptionButton,
    OptionSpinInteger,
    OptionSpinFloat,
    OptionCheck,
    OptionString,
    OptionCombo,
    OptionScore,
    OptionHalfScorepair,

    OPTION_TYPE_COUNT,
} OptionType;

typedef struct _OptButtonParams {
} OptButtonParams;

typedef struct _OptSpinIntegerParams {
    i64 *current_value;
    i64 default_value;
    i64 min_value;
    i64 max_value;
    bool is_tunable;
} OptSpinIntegerParams;

typedef struct _OptSpinFloatParams {
    f64 *current_value;
    f64 default_value;
    f64 min_value;
    f64 max_value;
    i64 resolution;
    bool is_tunable;
} OptSpinFloatParams;

typedef struct _OptCheckParams {
    bool *current_value;
    bool default_value;
} OptCheckParams;

typedef struct _OptStringParams {
    String *current_value;
    String default_value;
} OptStringParams;

typedef struct _OptComboParams {
    String *current_value;
    String default_value;
    String *allowed_values;
    usize allowed_count;
} OptComboParams;

typedef struct _OptScoreParams {
    Score *current_value;
    Score default_value;
    Score min_value;
    Score max_value;
    bool is_tunable;
} OptScoreParams;

typedef struct _OptHalfScorepairParams {
    Scorepair *current_value;
    Score default_value;
    Score min_value;
    Score max_value;
    Phase phase;
    bool is_tunable;
} OptHalfScorepairParams;

typedef union _OptionParams {
    OptButtonParams button;
    OptSpinIntegerParams spin_integer;
    OptSpinFloatParams spin_float;
    OptCheckParams check;
    OptStringParams string;
    OptComboParams combo;
    OptScoreParams score;
    OptHalfScorepairParams half_scorepair;
} OptionParams;

typedef struct _OptionVtable {
    void (*option_show)(StringView, const OptionParams *);
    void (*option_show_tune)(StringView, const OptionParams *);
    bool (*option_try_set)(OptionParams *, StringView);
    void (*option_dtor)(OptionParams *);
} OptionVtable;

typedef struct _Option {
    String option_name;
    OptionType option_type;
    const OptionVtable *option_vtable;
    void (*setoption_callback)(const OptionParams *, void *);
    OptionParams option_params;
    void *callback_data;
} Option;

typedef struct _OptionList {
    Option *options;
    usize size;
    usize capacity;
} OptionList;

// TODO: tuning macros are missing.

void optlist_init(OptionList *optlist);

void optlist_destroy(OptionList *optlist);

void optlist_show_options(const OptionList *optlist);

void optlist_show_tunable_options(const OptionList *optlist);

void optlist_set_option(OptionList *optlist, StringView name, StringView value);

void optlist_add_button(
    OptionList *optlist,
    StringView name,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

void optlist_add_spin_integer(
    OptionList *optlist,
    StringView name,
    i64 *value,
    i64 minval,
    i64 maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

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
);

void optlist_add_check(
    OptionList *optlist,
    StringView name,
    bool *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

void optlist_add_string(
    OptionList *optlist,
    StringView name,
    String *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

void optlist_add_combo(
    OptionList *optlist,
    StringView name,
    String *value,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data,
    usize allowed_count,
    ...
);

void optlist_add_score(
    OptionList *optlist,
    StringView name,
    Score *value,
    Score minval,
    Score maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

void optlist_add_scorepair(
    OptionList *optlist,
    StringView name,
    Scorepair *value,
    Score minval,
    Score maxval,
    bool is_tunable,
    void (*setoption_callback)(const OptionParams *, void *),
    void *callback_data
);

#endif
