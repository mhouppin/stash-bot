#include "option.h"

void    init_option_list(option_list_t *list)
{
    list->options = NULL;
    list->size = 0;
    list->max_size = 0;
}

void    quit_option_list(option_list_t *list)
{
    for (size_t i = 0; i < list->size; ++i)
    {
        option_t    *cur = &list->options[i];

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
                for (size_t k = 0; cur->combo_list[k]; ++k)
                    free(cur->combo_list[k]);
                free(cur->combo_list);

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
    list->max_size = 0;
}
