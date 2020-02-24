/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_d.c                                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 18:27:38 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 20:19:05 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include "board.h"

void	uci_d(const char *args)
{
	(void)args;
	extern const board_t	g_board;
	const char				*grid = "+---+---+---+---+---+---+---+---+";
	const char				*piece_to_char = " PNBRQK  pnbrqk";

	puts(grid);

	for (file_t rank = RANK_8; rank >= RANK_1; --rank)
	{
		for (file_t file = FILE_A; file <= FILE_H; ++file)
			printf("| %c ", piece_to_char[piece_on(&g_board,
				create_square(file, rank))]);

		puts("|");
		puts(grid);
	}
	printf("Eval: %d on midgame, %d on endgame\n",
		(int)midgame_score(g_board.psq_scorepair),
		(int)endgame_score(g_board.psq_scorepair));

	fflush(stdout);
}
