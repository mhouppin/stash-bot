/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_debug.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:09:14 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 05:33:53 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "settings.h"
#include <string.h>
#include <stdio.h>

void	uci_debug(const char *args)
{
	/*
	if (pthread_mutex_lock(&mtx_debug) != 0)
	{
		perror("Error while trying to set debug");
		return ;
	}
	*/

	if (strcmp(args, "off\n") == 0)
		g_debug = DEBUG_OFF;
	else if (strcmp(args, "on\n") == 0)
		g_debug = DEBUG_ON;

//	pthread_mutex_unlock(&mtx_debug);
}
