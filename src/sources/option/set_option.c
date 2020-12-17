#include "option.h"
#include <inttypes.h>
#include <stdio.h>

void    set_option(option_list_t *list, const char *name, const char *value)
{
    size_t  left = 0;
    size_t  right = list->size;
    size_t  i;

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
            option_t    *cur = &list->options[i];
            long        l;
            double      d;
            score_t     s;

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
