/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_quit.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 09:38:45 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 09:52:06 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <pthread.h>

void	uci_quit(const char *args)
{
	(void)args;
//	engine_exit(1);
	pthread_exit(NULL);
}
