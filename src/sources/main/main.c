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

#include "init.h"
#include "tt.h"
#include "uci.h"
#include <pthread.h>
#include <stdio.h>

int		main(int argc, char **argv)
{
	bitboard_init();
	psq_score_init();
	zobrist_init();
	tt_resize(16);

	pthread_t	engine_pt;

	if (pthread_create(&engine_pt, NULL, &engine_thread, NULL))
	{
		perror("Failed to boot engine thread");
		return (1);
	}

	wait_search_end();

	uci_loop(argc, argv);

	return (0);
}
