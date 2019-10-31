/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_d.c                                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 23:14:52 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 23:21:08 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <stdio.h>

void	uci_d(const char *args)
{
	(void)args;
	const char	*pretty = "+---+---+---+---+---+---+---+---+";
	const char	*pmagic = " PNBRQK\0\0pnbrqk";

	puts(pretty);

	for (int8_t rank = RANK_8; rank >= RANK_1; --rank)
	{
		for (int8_t file = FILE_A; file <= FILE_H; ++file)
		{
			printf("| %c ", pmagic[g_real_board.table[(rank << 3) + file]]);
		}
		puts("|");
		puts(pretty);
	}
}
