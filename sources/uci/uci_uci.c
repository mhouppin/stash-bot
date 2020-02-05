/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_uci.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:48:28 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/12/14 09:55:48 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <stdio.h>

void	uci_uci(const char *args)
{
	(void)args;
	puts("id name Stash v6.0");
	puts("id author Morgan Houppin (@mhouppin)");
	puts("option name Threads type spin default 1 min 1 max 32");
	puts("option name Hash type spin default 16 min 1 max 4096");
	puts("option name Clear Hash type button");
//	puts("option name MultiPV type spin default 1 min 1 max 16");
	puts("option name Minimum Thinking Time type spin default 20 min 0 max 30000");
	puts("uciok");
	fflush(stdout);
}
