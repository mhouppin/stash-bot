/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   out_of_time.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/04/01 12:59:28 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/04/01 13:24:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
