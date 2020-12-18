#ifndef OPTION_H
# define OPTION_H

# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include "score.h"

typedef enum
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
}
option_type_t;

// Warning: int spins are always treated as long
// and flt spins are always treated as double.

typedef struct
{
    char            *name;
    option_type_t   type;
    void            *data;
    void            (*callback)(void *);
    void            *def;
    void            *min;
    void            *max;
    char            **combo_list;
}
option_t;

typedef struct
{
    option_t        *options;
    size_t          size;
    size_t          max_size;
}
option_list_t;

extern option_list_t    g_opthandler;

void    init_option_list(option_list_t *list);
void    quit_option_list(option_list_t *list);

void    add_option_spin_int(option_list_t *list, const char *name, long *data,
        long min, long max, void (*callback)(void *));
void    add_option_spin_flt(option_list_t *list, const char *name, double *data,
        double min, double max, void (*callback)(void *));
void    add_option_check(option_list_t *list, const char *name, bool *data,
        void (*callback)(void *));
void    add_option_combo(option_list_t *list, const char *name, char **data,
        const char *const *combo_list, void (*callback)(void *));
void    add_option_button(option_list_t *list, const char *name,
        void (*callback)(void *));
void    add_option_string(option_list_t *list, const char *name, char **data,
        void (*callback)(void *));
void    add_option_score(option_list_t *list, const char *name, score_t *data,
        score_t min, score_t max, void (*callback)(void *));
void    add_option_scorepair(option_list_t *list, const char *name,
        scorepair_t *data, scorepair_t min, scorepair_t max,
        void (*callback)(void *));

void    show_options(const option_list_t *list);
void    set_option(option_list_t *list, const char *name, const char *value);

#endif
