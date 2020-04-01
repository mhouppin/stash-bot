/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_uci.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 19:47:11 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/04/01 13:25:21 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <stdio.h>

void	uci_uci(const char *args)
{
	(void)args;
	puts("id name Stash v13.0.1");
	puts("id author Morgan Houppin (@mhouppin)");
	puts("option name Hash type spin default 16 min 1 max 131072");
	puts("option name Clear Hash type button");
	puts("option name MultiPV type spin default 1 min 1 max 16");
	puts("option name Minimum Thinking Time type spin default 20 min 0 max 30000");
	puts("option name Move Overhead type spin default 20 min 0 max 1000");
	puts("option name UCI_Chess960 type check default false");
	puts("uciok");
	fflush(stdout);
}
