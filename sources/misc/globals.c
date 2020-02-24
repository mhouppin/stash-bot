/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   globals.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 18:36:25 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 20:11:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "info.h"
#include "uci.h"

board_t				g_board;
pthread_cond_t		g_engine_condvar;
enum e_egn_mode		g_engine_mode;
pthread_mutex_t		g_engine_mutex;
enum e_egn_send		g_engine_send;
goparams_t			g_goparams;
uint64_t			g_nodes;
ucioptions_t		g_options;
movelist_t			g_searchmoves;

void __attribute__((constructor))	init_globals(void)
{
	pthread_cond_init(&g_engine_condvar, NULL);
	pthread_mutex_init(&g_engine_mutex, NULL);

	g_engine_mode = WAITING;
	g_engine_send = DO_NOTHING;

	g_nodes = 0;

	g_options.hash = 16 * 1048576ul;
	g_options.move_overhead = 20;
	g_options.multi_pv = 1;
	g_options.min_think_time = 20;
}
