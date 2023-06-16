/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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
#include "uci.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// The function pointer table dispatcher for setting options.
bool (*const TrySetOptionList[OPTION_TYPE_COUNT])(Option *, const char *) = {
    &try_set_option_spin_int,
    &try_set_option_spin_flt,
    &try_set_option_check,
    &try_set_option_string,
    &try_set_option_combo,
    &try_set_option_button,
    &try_set_option_score,
    &try_set_option_scorepair,
    &try_set_option_scorepair,
};

void option_allocation_failure(void)
{
    perror("Unable to allocate option table");
    exit(EXIT_FAILURE);
}

void init_option_list(OptionList *list)
{
    list->options = NULL;
    list->size = 0;
    list->maxSize = 0;
}

void quit_option_list(OptionList *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        Option *cur = &list->options[i];

        free(cur->name);

        switch (cur->type)
        {
            case OptionSpinInt:
            case OptionSpinFlt:
            case OptionScore:
            case OptionSpairMG:
            case OptionSpairEG:
                // All these option types require default/min/max values.
                free(cur->def);
                free(cur->min);
                free(cur->max);
                break;

            case OptionCombo:
                for (size_t k = 0; cur->comboList[k]; ++k) free(cur->comboList[k]);
                free(cur->comboList);
                // Fallthrough

            case OptionString:
                free(*(char **)cur->data);
                *(char **)cur->data = NULL;
                break;

            case OptionCheck: free(cur->def); break;

            case OptionButton: break;

            // Safeguard
            default: break;
        }
    }

    free(list->options);
    list->options = NULL;
    list->maxSize = 0;
}

Option *insert_option(OptionList *list, const char *name)
{
    // Extend the option buffer if we're running out of empty slots.
    if (list->size == list->maxSize)
    {
        list->maxSize += (!list->maxSize) ? 16 : list->maxSize;
        list->options = realloc(list->options, list->maxSize * sizeof(Option));
        if (!list->options) option_allocation_failure();
    }

    // Do a binary search to find the index of our new option.
    size_t left = 0;
    size_t right = list->size;

    while (left < right)
    {
        size_t i = (left + right) / 2;
        if (strcasecmp(name, list->options[i].name) < 0)
            right = i;
        else
            left = i + 1;
    }

    // Make the space to insert the new option.
    memmove(&list->options[left + 1], &list->options[left], sizeof(Option) * (list->size - left));
    memset(&list->options[left], 0, sizeof(Option));

    list->options[left].name = strdup(name);

    if (!list->options[left].name) option_allocation_failure();

    list->size++;

    // Return the option pointer to the caller.
    return &list->options[left];
}

void add_option_spin_int(
    OptionList *list, const char *name, long *data, long min, long max, void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionSpinInt;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(long));
    cur->min = malloc(sizeof(long));
    cur->max = malloc(sizeof(long));
    if (!cur->def || !cur->min || !cur->max) option_allocation_failure();

    *(long *)cur->def = *data;
    *(long *)cur->min = min;
    *(long *)cur->max = max;
}

void add_option_spin_flt(OptionList *list, const char *name, double *data, double min, double max,
    void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionSpinFlt;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(double));
    cur->min = malloc(sizeof(double));
    cur->max = malloc(sizeof(double));
    if (!cur->def || !cur->min || !cur->max) option_allocation_failure();

    *(double *)cur->def = *data;
    *(double *)cur->min = min;
    *(double *)cur->max = max;
}

void add_option_score(OptionList *list, const char *name, score_t *data, score_t min, score_t max,
    void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionScore;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max) option_allocation_failure();

    *(score_t *)cur->def = *data;
    *(score_t *)cur->min = min;
    *(score_t *)cur->max = max;
}

void add_option_scorepair(OptionList *list, const char *name, scorepair_t *data, scorepair_t min,
    scorepair_t max, void (*callback)(void *))
{
    // We register scorepair options as two separated options, one for the
    // middlegame value with a "MG" suffix, and one for the endgame value with
    // the "EG" suffix. Both operate on the same scorepair pointer internally.
    char *buffer = malloc(strlen(name) + 3);

    if (!buffer) option_allocation_failure();

    // Start by initializing the middlegame option.
    strcpy(buffer, name);
    strcat(buffer, "MG");

    Option *cur = insert_option(list, buffer);

    cur->type = OptionSpairMG;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max) option_allocation_failure();

    *(score_t *)cur->def = midgame_score(*data);
    *(score_t *)cur->min = midgame_score(min);
    *(score_t *)cur->max = midgame_score(max);

    // Then initialize the endgame option.
    buffer[strlen(buffer) - 2] = 'E';

    cur = insert_option(list, buffer);

    cur->type = OptionSpairEG;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max) option_allocation_failure();

    *(score_t *)cur->def = endgame_score(*data);
    *(score_t *)cur->min = endgame_score(min);
    *(score_t *)cur->max = endgame_score(max);
    free(buffer);
}

void add_option_check(OptionList *list, const char *name, bool *data, void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionCheck;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(bool));
    if (!cur->def) option_allocation_failure();
    *(bool *)cur->def = *data;
}

void add_option_combo(OptionList *list, const char *name, char **data, const char *const *comboList,
    void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionCombo;
    cur->data = data;
    cur->callback = callback;
    cur->def = strdup(*data ? *data : "");

    if (!cur->def) option_allocation_failure();

    size_t length;
    for (length = 0; comboList[length]; ++length)
        ;

    cur->comboList = malloc(sizeof(char *) * (length + 1));
    if (!cur->comboList) option_allocation_failure();

    for (size_t i = 0; i < length; ++i)
    {
        cur->comboList[i] = strdup(comboList[i]);
        if (!cur->comboList[i]) option_allocation_failure();
    }
    cur->comboList[length] = NULL;
}

void add_option_button(OptionList *list, const char *name, void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionButton;
    cur->callback = callback;
}

void add_option_string(OptionList *list, const char *name, char **data, void (*callback)(void *))
{
    Option *cur = insert_option(list, name);

    cur->type = OptionString;
    cur->data = data;
    cur->callback = callback;
    cur->def = strdup(*data ? *data : "");

    if (!cur->def) option_allocation_failure();
}

bool try_set_option_spin_int(Option *option, const char *value)
{
    char *endptr;
    long ivalue = strtol(value, &endptr, 10);

    if (endptr == value)
    {
        debug_printf(
            "info error Failed to parse value '%s' for option '%s'\n", value, option->name);
        return false;
    }

    if (ivalue < *(long *)option->min || ivalue > *(long *)option->max)
    {
        debug_printf("info error Value '%s' falls outside of the supported range for option '%s'\n",
            value, option->name);
        return false;
    }

    *(long *)option->data = ivalue;
    debug_printf("info string Setting option '%s' to %ld\n", option->name, ivalue);
    return true;
}

bool try_set_option_spin_flt(Option *option, const char *value)
{
    char *endptr;
    double fvalue = strtod(value, &endptr);

    if (endptr == value)
    {
        debug_printf(
            "info error Failed to parse value '%s' for option '%s'\n", value, option->name);
        return false;
    }

    if (fvalue < *(double *)option->min || fvalue > *(double *)option->max)
    {
        debug_printf("info error Value '%s' falls outside of the supported range for option '%s'\n",
            value, option->name);
        return false;
    }

    *(double *)option->data = fvalue;
    debug_printf("info string Setting option '%s' to %lg\n", option->name, fvalue);
    return true;
}

bool try_set_option_score(Option *option, const char *value)
{
    char *endptr;
    long ivalue = strtol(value, &endptr, 10);

    if (endptr == value)
    {
        debug_printf(
            "info error Failed to parse value '%s' for option '%s'\n", value, option->name);
        return false;
    }

    if (ivalue < (long)*(score_t *)option->min || ivalue > (long)*(score_t *)option->max)
    {
        debug_printf("info error Value '%s' falls outside of the supported range for option '%s'\n",
            value, option->name);
        return false;
    }

    *(score_t *)option->data = (score_t)ivalue;
    debug_printf("info string Setting option '%s' to %ld\n", option->name, ivalue);
    return true;
}

bool try_set_option_scorepair(Option *option, const char *value)
{
    char *endptr;
    long ivalue = strtol(value, &endptr, 10);

    if (endptr == value)
    {
        debug_printf(
            "info error Failed to parse value '%s' for option '%s'\n", value, option->name);
        return false;
    }

    if (ivalue < (long)*(score_t *)option->min || ivalue > (long)*(score_t *)option->max)
    {
        debug_printf("info error Value '%s' falls outside of the supported range for option '%s'\n",
            value, option->name);
        return false;
    }

    {
        scorepair_t *spair = (scorepair_t *)option->data;

        if (option->type == OptionSpairMG)
            *spair = create_scorepair((score_t)ivalue, endgame_score(*spair));
        else
            *spair = create_scorepair(midgame_score(*spair), (score_t)ivalue);
    }

    debug_printf("info string Setting option '%s' to %ld\n", option->name, ivalue);
    return true;
}

bool try_set_option_check(Option *option, const char *value)
{
    if (strcasecmp(value, "true") == 0)
    {
        *(bool *)option->data = true;
        debug_printf("info string Setting option '%s' to true\n", option->name);
    }
    else if (strcasecmp(value, "false") == 0)
    {
        *(bool *)option->data = false;
        debug_printf("info string Setting option '%s' to false\n", option->name);
    }
    else
    {
        debug_printf(
            "info error Value '%s' for option '%s' is not a boolean\n", value, option->name);
        return false;
    }

    return true;
}

bool try_set_option_string(Option *option, const char *value)
{
    char *vcopy = strdup(value);

    if (vcopy == NULL)
    {
        debug_printf("info error Unable to set value '%s' for option '%s': %s\n", value,
            option->name, strerror(errno));
        return false;
    }

    free(*(char **)option->data);
    *(char **)option->data = vcopy;
    debug_printf("info string Setting option '%s' to '%s'\n", option->name, value);
    return true;
}

bool try_set_option_combo(Option *option, const char *value)
{
    for (size_t i = 0; option->comboList[i]; ++i)
        if (strcasecmp(option->comboList[i], value) == 0)
            return try_set_option_string(option, option->comboList[i]);

    debug_printf("info error value '%s' is not in the list of supported values for option '%s'\n",
        value, option->name);
    return false;
}

bool try_set_option_button(Option *option, const char *value)
{
    (void)value;
    debug_printf("info string Setting option '%s'\n", option->name);
    return true;
}

void set_option(OptionList *list, const char *name, const char *value)
{
    size_t left = 0;
    size_t right = list->size;

    // Search the option by doing a binary search on the option's name.
    while (left < right)
    {
        size_t i = (left + right) / 2;

        int result = strcasecmp(name, list->options[i].name);

        if (result < 0)
            right = i;
        else if (result > 0)
            left = i + 1;
        else
        {
            Option *cur = &list->options[i];

            // Perform a check to see if the passed value can be assigned to the
            // option. That means falling in the accepted range for integers and
            // floats, being "true/false" for booleans, and being one of the
            // accepted names for combo lists.
            bool set_success = TrySetOptionList[cur->type](cur, value);

            // Only call bound functions if the option setting was successful.
            if (set_success && cur->callback) cur->callback(cur->data);

            return;
        }
    }

    debug_printf("info error Unknown option '%s'\n", name);
}

void show_options(const OptionList *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        const Option *cur = &list->options[i];

        switch (cur->type)
        {
            case OptionSpinInt:
                printf("option name %s type spin default %ld min %ld max %ld\n", cur->name,
                    *(long *)cur->def, *(long *)cur->min, *(long *)cur->max);
                break;

            case OptionSpinFlt:
                // Tricky case: spins can't be floats, so we show them as strings and
                // handle them internally.
                printf("option name %s type string default %lf\n", cur->name, *(double *)cur->def);
                break;

            case OptionScore:
            case OptionSpairMG:
            case OptionSpairEG:
                printf("option name %s type spin default %" PRId16 " min %" PRId16 " max %" PRId16
                       "\n",
                    cur->name, *(score_t *)cur->def, *(score_t *)cur->min, *(score_t *)cur->max);
                break;

            case OptionCheck:
                printf("option name %s type check default %s\n", cur->name,
                    *(bool *)cur->def ? "true" : "false");
                break;

            case OptionCombo:
                printf("option name %s type combo default %s", cur->name, (char *)cur->def);

                for (size_t k = 0; cur->comboList[k]; ++k) printf(" var %s", cur->comboList[k]);
                puts("");
                break;

            case OptionButton: printf("option name %s type button\n", cur->name); break;

            case OptionString:
                printf("option name %s type string default %s\n", cur->name, (char *)cur->def);
                break;

            // Safeguard
            default: break;
        }
    }
    fflush(stdout);
}
