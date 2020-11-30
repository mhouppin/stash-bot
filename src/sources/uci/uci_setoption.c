/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

#include "tt.h"
#include "uci.h"
#include "option.h"
#include <string.h>
#include <stdlib.h>

void    uci_setoption(const char *args)
{
    if (!args)
        return ;

    const char  *delim = " \t\n";

    char    *copy = strdup(args);
    char    *name_token = strstr(copy, "name");
    char    *value_token = strstr(copy, "value");

    if (!name_token)
    {
        free(copy);
        return ;
    }

    char    namebuf[1024] = {0};
    char    valuebuf[1024];

    if (value_token)
    {
        *(value_token - 1) = '\0';
        strcpy(valuebuf, value_token + 6);

        // Remove the final newline to valuebuf
        char    *maybe_nl = &valuebuf[strlen(valuebuf) - 1];
        if (*maybe_nl == '\n')
            *maybe_nl = '\0';
    }
    else
        valuebuf[0] = '\0';

    char    *token;

    token = strtok(name_token + 4, delim);
    while (token)
    {
        strcat(namebuf, token);
        token = strtok(NULL, delim);
        if (token)
            strcat(namebuf, " ");
    }

    set_option(&g_opthandler, namebuf, valuebuf);
    free(copy);
}
