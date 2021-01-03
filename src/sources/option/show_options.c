#include "option.h"
#include <inttypes.h>
#include <stdio.h>

void    show_options(const option_list_t *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        const option_t  *cur = &list->options[i];

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

                for (size_t i = 0; cur->combo_list[i]; ++i)
                    printf(" var %s", cur->combo_list[i]);
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
