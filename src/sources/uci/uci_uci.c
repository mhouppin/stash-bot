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

#include "uci.h"
#include <stdio.h>

void	uci_uci(const char *args)
{
	(void)args;
	puts("id name Stash v21.0-obfix");
	puts("id author Morgan Houppin (@mhouppin)");
	puts("option name Hash type spin default 16 min 1 max 131072");
	puts("option name Clear Hash type button");
	puts("option name MultiPV type spin default 1 min 1 max 16");
	puts("option name Minimum Thinking Time type spin default 20 min 0 max 30000");
	puts("option name Move Overhead type spin default 20 min 0 max 1000");
	puts("option name UCI_Chess960 type check default false");
	puts("uciok");
	fflush(stdout);
}
