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


#include "info.h"
#include "option.h"
#include "uci.h"

board_t				g_board;
pthread_cond_t		g_engine_condvar;
enum e_egn_mode		g_engine_mode;
pthread_mutex_t		g_engine_mutex;
enum e_egn_send		g_engine_send;
goparams_t			g_goparams;
option_list_t		g_opthandler;
ucioptions_t		g_options;
movelist_t			g_searchmoves;
uint64_t			g_seed;

void __attribute__((constructor))	init_globals(void)
{
	pthread_cond_init(&g_engine_condvar, NULL);
	pthread_mutex_init(&g_engine_mutex, NULL);

	g_engine_mode = THINKING;
	g_engine_send = DO_NOTHING;

	g_nodes = 0;

	g_options.threads = 1;
	g_options.hash = 16;
	g_options.move_overhead = 40;
	g_options.multi_pv = 1;
	g_options.burn_ratio = 1.2;
	g_options.save_ratio = 1.2;
	g_options.chess960 = false;

	g_seed = 1048592ul;
}
