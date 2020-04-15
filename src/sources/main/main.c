/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   main.c                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 18:55:11 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:40:26 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

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
	uci_position("startpos");

	pthread_t	engine_pt;

	if (pthread_create(&engine_pt, NULL, &engine_thread, NULL))
	{
		perror("Failed to boot engine thread");
		return (1);
	}
	uci_loop(argc, argv);

	return (0);
}
