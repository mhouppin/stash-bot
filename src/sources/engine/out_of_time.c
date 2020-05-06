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

#include "engine.h"
#include "uci.h"

bool	out_of_time(void)
{
	extern uint64_t		g_nodes;
	extern goparams_t	g_goparams;

	pthread_mutex_lock(&g_engine_mutex);
	enum e_egn_send	send = g_engine_send;
	pthread_mutex_unlock(&g_engine_mutex);

	// Check if a "quit" or "stop" command has been received.

	if (send == DO_ABORT || send == DO_EXIT)
		return (true);

	// Don't use time management when "go infinite" has been used.

	if (g_goparams.infinite)
		return (false);

	if (g_nodes >= g_goparams.nodes)
		return (true);

	if (g_goparams.movetime || g_goparams.wtime || g_goparams.winc
		|| g_goparams.btime || g_goparams.binc)
	{
		clock_t		end = g_goparams.start + g_goparams.max_time;

		if (chess_clock() > end)
			return (true);
	}

	return (false);
}
