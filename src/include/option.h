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

#include "types.h"
#include <stdlib.h>
#include <string.h>

// Enum for supported option types
typedef enum option_type_e
{
    OptionSpinInt,
    OptionSpinFlt,
    OptionCheck,
    OptionString,
    OptionCombo,
    OptionButton,
    OptionScore,
    OptionSpairMG,
    OptionSpairEG,
    OPTION_TYPE_COUNT
} option_type_t;

// Some helper macros for tuning stuff

#define TUNE_SCORE(x, minval, maxval)                                   \
    do {                                                                \
        extern score_t x;                                               \
        add_option_score(&UciOptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

#define TUNE_SP(x, minval, maxval)                                                       \
    do {                                                                                 \
        extern scorepair_t x;                                                            \
        add_option_scorepair(                                                            \
            &UciOptionList, #x, &x, SPAIR(minval, minval), SPAIR(maxval, maxval), NULL); \
    } while (0);

#define TUNE_SP_ARRAY(x, len, start, end, minval, maxval)                               \
    do {                                                                                \
        extern scorepair_t x[len];                                                      \
        char __buf[128];                                                                \
        for (int __i = start; __i < end; ++__i)                                         \
        {                                                                               \
            snprintf(__buf, 128, #x "_%02d", __i);                                      \
            add_option_scorepair(&UciOptionList, __buf, &x[__i], SPAIR(minval, minval), \
                SPAIR(maxval, maxval), NULL);                                           \
        }                                                                               \
    } while (0);

#define TUNE_LONG(x, minval, maxval)                                       \
    do {                                                                   \
        extern long x;                                                     \
        add_option_spin_int(&UciOptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

#define TUNE_BOOL(x)                                    \
    do {                                                \
        extern bool x;                                  \
        add_option_check(&UciOptionList, #x, &x, NULL); \
    } while (0);

#define TUNE_DOUBLE(x, minval, maxval)                                     \
    do {                                                                   \
        extern double x;                                                   \
        add_option_spin_flt(&UciOptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

// Warning: int spins are always treated as long
// and flt spins are always treated as double.

// Struct for an option
typedef struct _Option
{
    char *name;
    option_type_t type;
    void *data;
    void (*callback)(void *);
    void *def;
    void *min;
    void *max;
    char **comboList;
} Option;

// Struct for a list of options
typedef struct _OptionList
{
    Option *options;
    size_t size;
    size_t maxSize;
} OptionList;

// Global option list
extern OptionList UciOptionList;

// Initializes the option list.
void init_option_list(OptionList *list);

// Frees all memory associated with the option list.
void quit_option_list(OptionList *list);

// Creates a new option of type `spin` holding a long and adds it to the option list.
void add_option_spin_int(
    OptionList *list, const char *name, long *data, long min, long max, void (*callback)(void *));

// Creates a new option of type `string` holding a double and adds it to the option list.
void add_option_spin_flt(OptionList *list, const char *name, double *data, double min, double max,
    void (*callback)(void *));

// Creates a new option of type `check` and adds it to the option list.
void add_option_check(OptionList *list, const char *name, bool *data, void (*callback)(void *));

// Creates a new option of type `combo` and adds it to the option list.
void add_option_combo(OptionList *list, const char *name, char **data, const char *const *comboList,
    void (*callback)(void *));

// Creates a new option of type `button` and adds it to the option list.
void add_option_button(OptionList *list, const char *name, void (*callback)(void *));

// Creates a new option of type `string` and adds it to the option list.
void add_option_string(OptionList *list, const char *name, char **data, void (*callback)(void *));

// Creates a new option of type `spin` holding a score and adds it to the option list.
void add_option_score(OptionList *list, const char *name, score_t *data, score_t min, score_t max,
    void (*callback)(void *));

// Creates two new options of type `spin` holding a scorepair and adds them to the option list.
void add_option_scorepair(OptionList *list, const char *name, scorepair_t *data, scorepair_t min,
    scorepair_t max, void (*callback)(void *));

// Displays the option list as specified by the UCI protocol.
void show_options(const OptionList *list);

// Sets the value of an option.
void set_option(OptionList *list, const char *name, const char *value);

// All functions that try to parse a value for a specific option type, and return whether the
// operation succeeded or not.
bool try_set_option_spin_int(Option *option, const char *value);
bool try_set_option_spin_flt(Option *option, const char *value);
bool try_set_option_check(Option *option, const char *value);
bool try_set_option_string(Option *option, const char *value);
bool try_set_option_combo(Option *option, const char *value);
bool try_set_option_button(Option *option, const char *value);
bool try_set_option_score(Option *option, const char *value);
bool try_set_option_scorepair(Option *option, const char *value);

#endif // OPTION_H
