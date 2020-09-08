/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include "uci.h"

char	*get_next_token(char **str)
{
	while (isspace(**str) && **str != '\0')
		++(*str);

	if (**str == '\0')
		return (NULL);

	char	*retval = *str;

	while (!isspace(**str) && **str != '\0')
		++(*str);

	if (**str == '\0')
		return (retval);

	**str = '\0';
	++(*str);
	return (retval);
}
