/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   main.c                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:24:59 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 23:41:57 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <pthread.h>
#include <stdio.h>

int		main(void)
{
	pthread_t	uci_pt;
	pthread_t	engine_pt;

	if (pthread_create(&uci_pt, NULL, &uci_thread, NULL))
	{
		perror("Failed to boot UCI thread");
		return (2);
	}
	if (pthread_create(&engine_pt, NULL, &engine_thread, NULL))
	{
		perror("Failed to boot engine thread");
		return (2);
	}
	if (pthread_join(uci_pt, NULL))
	{
		perror("Failed to wait for UCI thread");
		return (2);
	}
	pthread_cancel(engine_pt);
	return (0);
}
