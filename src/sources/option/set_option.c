#include "option.h"
#include <stdio.h>

void	set_option(option_list_t *list, const char *name, const char *value)
{
	size_t	left = 0;
	size_t	right = list->size;
	size_t	i;

	while (left < right)
	{
		i = (left + right) / 2;

		int		result = strcmp(name, list->options[i].name);

		if (result < 0)
			right = i;
		else if (result > 0)
			left = i + 1;
		else
		{
			option_t	*cur = &list->options[i];
			long		l;
			double		d;

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
