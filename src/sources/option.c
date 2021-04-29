/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#include <inttypes.h>
#include <stdio.h>
#include "option.h"

void option_allocation_failure(void)
{
    perror("Unable to allocate option table");
    exit(EXIT_FAILURE);
}

void init_option_list(option_list_t *list)
{
    list->options = NULL;
    list->size = 0;
    list->maxSize = 0;
}

void quit_option_list(option_list_t *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        option_t *cur = &list->options[i];

        free(cur->name);

        switch (cur->type)
        {
            case OptionSpinInt:
            case OptionSpinFlt:
            case OptionScore:
            case OptionSpairMG:
            case OptionSpairEG:
                free(cur->def);
                free(cur->min);
                free(cur->max);
                break ;

            case OptionCombo:
                for (size_t k = 0; cur->comboList[k]; ++k)
                    free(cur->comboList[k]);
                free(cur->comboList);
                // Fallthrough

            case OptionString:
                free(*(char **)cur->data);
                *(char **)cur->data = NULL;
                break ;

            case OptionCheck:
                free(cur->def);
                break ;

            case OptionButton:
                break ;
        }
    }

    free(list->options);
    list->options = NULL;
    list->maxSize = 0;
}

option_t *insert_option(option_list_t *list, const char *name)
{
    if (list->size == list->maxSize)
    {
        list->maxSize += (!list->maxSize) ? 16 : list->maxSize;
        list->options = realloc(list->options, list->maxSize * sizeof(option_t));
        if (!list->options)
            option_allocation_failure();
    }

    // Binary search to find the index of our new option.

    size_t left = 0;
    size_t right = list->size;
    size_t i;

    while (left < right)
    {
        i = (left + right) / 2;
        if (strcmp(name, list->options[i].name) < 0)
            right = i;
        else
            left = i + 1;
    }
    memmove(&list->options[left + 1], &list->options[left], sizeof(option_t) * (list->size - left));
    memset(&list->options[left], 0, sizeof(option_t));

    list->options[left].name = strdup(name);

    if (!list->options[left].name)
        option_allocation_failure();

    list->size++;

    return (&list->options[left]);
}

void add_option_spin_int(option_list_t *list, const char *name, long *data, long min, long max,
    void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionSpinInt;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(long));
    cur->min = malloc(sizeof(long));
    cur->max = malloc(sizeof(long));
    if (!cur->def || !cur->min || !cur->max)
        option_allocation_failure();

    *(long *)cur->def = *data;
    *(long *)cur->min = min;
    *(long *)cur->max = max;
}

void add_option_spin_flt(option_list_t *list, const char *name, double *data, double min, double max,
    void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionSpinFlt;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(double));
    cur->min = malloc(sizeof(double));
    cur->max = malloc(sizeof(double));
    if (!cur->def || !cur->min || !cur->max)
        option_allocation_failure();

    *(double *)cur->def = *data;
    *(double *)cur->min = min;
    *(double *)cur->max = max;
}

void add_option_score(option_list_t *list, const char *name, score_t *data, score_t min, score_t max,
    void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionScore;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max)
        option_allocation_failure();

    *(score_t *)cur->def = *data;
    *(score_t *)cur->min = min;
    *(score_t *)cur->max = max;
}

void add_option_scorepair(option_list_t *list, const char *name, scorepair_t *data, scorepair_t min, scorepair_t max,
    void (*callback)(void *))
{
    char *buffer = malloc(strlen(name) + 3);

    if (!buffer)
        option_allocation_failure();

    strcpy(buffer, name);
    strcat(buffer, "MG");

    option_t *cur = insert_option(list, buffer);

    cur->type = OptionSpairMG;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max)
        option_allocation_failure();

    *(score_t *)cur->def = midgame_score(*data);
    *(score_t *)cur->min = midgame_score(min);
    *(score_t *)cur->max = midgame_score(max);

    buffer[strlen(buffer) - 2] = 'E';

    cur = insert_option(list, buffer);

    cur->type = OptionSpairEG;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(score_t));
    cur->min = malloc(sizeof(score_t));
    cur->max = malloc(sizeof(score_t));
    if (!cur->def || !cur->min || !cur->max)
        option_allocation_failure();

    *(score_t *)cur->def = endgame_score(*data);
    *(score_t *)cur->min = endgame_score(min);
    *(score_t *)cur->max = endgame_score(max);
    free(buffer);
}

void add_option_check(option_list_t *list, const char *name, bool *data, void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionCheck;
    cur->data = data;
    cur->callback = callback;
    cur->def = malloc(sizeof(bool));
    if (!cur->def)
        option_allocation_failure();
    *(bool *)cur->def = *data;
}

void add_option_combo(option_list_t *list, const char *name, char **data, const char *const *comboList,
    void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionCombo;
    cur->data = data;
    cur->callback = callback;
    cur->def = strdup(*data ? *data : "");

    if (!cur->def)
        option_allocation_failure();

    size_t length;
    for (length = 0; comboList[length]; ++length);

    cur->comboList = malloc(sizeof(char *) * (length + 1));
    if (!cur->comboList)
        option_allocation_failure();

    for (size_t i = 0; i < length; ++i)
    {
        cur->comboList[i] = strdup(comboList[i]);
        if (!cur->comboList[i])
            option_allocation_failure();
    }
    cur->comboList[length] = NULL;
}

void add_option_button(option_list_t *list, const char *name, void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionButton;
    cur->callback = callback;
}

void add_option_string(option_list_t *list, const char *name, char **data, void (*callback)(void *))
{
    option_t *cur = insert_option(list, name);

    cur->type = OptionString;
    cur->data = data;
    cur->callback = callback;
    cur->def = strdup(*data ? *data : "");

    if (!cur->def)
        option_allocation_failure();
}

void set_option(option_list_t *list, const char *name, const char *value)
{
    size_t left = 0;
    size_t right = list->size;
    size_t i;

    while (left < right)
    {
        i = (left + right) / 2;

        int result = strcmp(name, list->options[i].name);

        if (result < 0)
            right = i;
        else if (result > 0)
            left = i + 1;
        else
        {
            option_t *cur = &list->options[i];
            long l;
            double d;
            score_t s;

            switch (cur->type)
            {
                case OptionSpinInt:
                    sscanf(value, "%ld", &l);
                    if (l >= *(long *)cur->min && l <= *(long *)cur->max)
                        *(long *)cur->data = l;
                    break ;

                case OptionSpinFlt:
                    sscanf(value, "%lf", &d);
                    if (d >= *(double *)cur->min && d <= *(double *)cur->max)
                        *(double *)cur->data = d;
                    break ;

                case OptionScore:
                    sscanf(value, "%" SCNd16, &s);
                    if (s >= *(score_t *)cur->min && s <= *(score_t *)cur->max)
                        *(score_t *)cur->data = s;
                    break ;

                case OptionSpairMG:
                    sscanf(value, "%" SCNd16, &s);
                    if (s >= *(score_t *)cur->min && s <= *(score_t *)cur->max)
                        *(scorepair_t *)cur->data = create_scorepair(s, endgame_score(*(scorepair_t *)cur->data));
                    break ;

                case OptionSpairEG:
                    sscanf(value, "%" SCNd16, &s);
                    if (s >= *(score_t *)cur->min && s <= *(score_t *)cur->max)
                        *(scorepair_t *)cur->data = create_scorepair(midgame_score(*(scorepair_t *)cur->data), s);
                    break ;

                case OptionCheck:
                    *(bool *)cur->data = !strcmp(value, "true");
                    break ;

                case OptionCombo:
                case OptionString:
                    free(*(char **)cur->data);
                    *(char **)cur->data = strdup(value);
                    if (!*(char **)cur->data)
                    {
                        perror("Unable to set option");
                        exit(EXIT_FAILURE);
                    }
                    break ;

                case OptionButton:
                    break ;
            }

            if (cur->callback)
                cur->callback(cur->data);

            return ;
        }
    }
}

void show_options(const option_list_t *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        const option_t *cur = &list->options[i];

        switch (cur->type)
        {
            case OptionSpinInt:
                printf("option name %s type spin default %ld min %ld max %ld\n",
                    cur->name, *(long *)cur->def, *(long *)cur->min, *(long *)cur->max);
                break ;

            // Tricky case: spins can't be floats, so we show them as strings and
            // handle them internally.

            case OptionSpinFlt:
                printf("option name %s type string default %lf\n",
                    cur->name, *(double *)cur->def);
                break ;

            case OptionScore:
            case OptionSpairMG:
            case OptionSpairEG:
                printf("option name %s type spin default %" PRId16 " min %" PRId16 " max %" PRId16 "\n",
                    cur->name, *(score_t *)cur->def, *(score_t *)cur->min, *(score_t *)cur->max);
                break ;

            case OptionCheck:
                printf("option name %s type check default %s\n",
                    cur->name, *(bool *)cur->def ? "true" : "false");
                break ;

            case OptionCombo:
                printf("option name %s type combo default %s",
                    cur->name, (char *)cur->def);

                for (size_t k = 0; cur->comboList[k]; ++k)
                    printf(" var %s", cur->comboList[k]);
                puts("");
                break ;

            case OptionButton:
                printf("option name %s type button\n", cur->name);
                break ;

            case OptionString:
                printf("option name %s type string default %s\n",
                    cur->name, (char *)cur->def);
                break ;
        }
    }
    fflush(stdout);
}
