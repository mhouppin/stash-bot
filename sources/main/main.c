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
#include <pthread.h>
#include <stdio.h>

int		main(void)
{
	bitboard_init();
	psq_score_init();
	zobrist_init();
	tt_resize(16);

	pthread_t	uci_pt;
	pthread_t	engine_pt;

	if (pthread_create(&uci_pt, NULL, &uci_thread, NULL))
	{
		perror("Failed to boot UCI thread");
		return (1);
	}
	if (pthread_create(&engine_pt, NULL, &engine_thread, NULL))
	{
		perror("Failed to boot engine thread");
		return (1);
	}
	if (pthread_join(uci_pt, NULL))
	{
		perror("Failed to wait for UCI thread");
		return (1);
	}
	if (pthread_join(engine_pt, NULL))
	{
		perror("Failed to wait for engine thread");
		return (1);
	}

	return (0);
}
