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

#ifndef OPTION_H
#define OPTION_H

#include "types.h"
#include <stdlib.h>
#include <string.h>

typedef enum option_type_e
{
    OptionSpinInt,
    OptionSpinFlt,
    OptionCheck,
    OptionCombo,
    OptionButton,
    OptionString,
    OptionScore,
    OptionSpairMG,
    OptionSpairEG
} option_type_t;

// Some helper macros for tuning stuff

#define TUNE_SCORE(x, minval, maxval)                                \
    do {                                                             \
        extern score_t x;                                            \
        add_option_score(&OptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

#define TUNE_SP(x, minval, maxval)                                                    \
    do {                                                                              \
        extern scorepair_t x;                                                         \
        add_option_scorepair(                                                         \
            &OptionList, #x, &x, SPAIR(minval, minval), SPAIR(maxval, maxval), NULL); \
    } while (0);

#define TUNE_SP_ARRAY(x, len, start, end, minval, maxval)                                         \
    do {                                                                                          \
        extern scorepair_t x[len];                                                                \
        char __buf[128];                                                                          \
        for (int __i = start; __i < end; ++__i)                                                   \
        {                                                                                         \
            sprintf(__buf, #x "_%02d", __i);                                                      \
            add_option_scorepair(                                                                 \
                &OptionList, __buf, &x[__i], SPAIR(minval, minval), SPAIR(maxval, maxval), NULL); \
        }                                                                                         \
    } while (0);

#define TUNE_LONG(x, minval, maxval)                                    \
    do {                                                                \
        extern long x;                                                  \
        add_option_spin_int(&OptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

#define TUNE_BOOL(x)                                 \
    do {                                             \
        extern bool x;                               \
        add_option_check(&OptionList, #x, &x, NULL); \
    } while (0);

#define TUNE_DOUBLE(x, minval, maxval)                                  \
    do {                                                                \
        extern double x;                                                \
        add_option_spin_flt(&OptionList, #x, &x, minval, maxval, NULL); \
    } while (0);

// Warning: int spins are always treated as long
// and flt spins are always treated as double.

typedef struct option_s
{
    char *name;
    option_type_t type;
    void *data;
    void (*callback)(void *);
    void *def;
    void *min;
    void *max;
    char **comboList;
} option_t;

typedef struct option_list_s
{
    option_t *options;
    size_t size;
    size_t maxSize;
} option_list_t;

extern option_list_t OptionList;

void init_option_list(option_list_t *list);
void quit_option_list(option_list_t *list);

void add_option_spin_int(option_list_t *list, const char *name, long *data, long min, long max,
    void (*callback)(void *));
void add_option_spin_flt(option_list_t *list, const char *name, double *data, double min,
    double max, void (*callback)(void *));
void add_option_check(option_list_t *list, const char *name, bool *data, void (*callback)(void *));
void add_option_combo(option_list_t *list, const char *name, char **data,
    const char *const *comboList, void (*callback)(void *));
void add_option_button(option_list_t *list, const char *name, void (*callback)(void *));
void add_option_string(
    option_list_t *list, const char *name, char **data, void (*callback)(void *));
void add_option_score(option_list_t *list, const char *name, score_t *data, score_t min,
    score_t max, void (*callback)(void *));
void add_option_scorepair(option_list_t *list, const char *name, scorepair_t *data, scorepair_t min,
    scorepair_t max, void (*callback)(void *));

void show_options(const option_list_t *list);
void set_option(option_list_t *list, const char *name, const char *value);

#endif // OPTION_H
