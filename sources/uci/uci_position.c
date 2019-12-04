/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_position.c                                   .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 09:38:19 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/12/04 20:46:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <string.h>
#include <stdlib.h>

void	uci_position(const char *args)
{
	char	*copy = strdup(args);
	char	*token;

	token = strtok(copy, " \t\n");

	if (strcmp(token, "fen") == 0)
	{
		int8_t	square = 0;

		token = strtok(NULL, " \t\n");

		memset(&g_init_board.table, '\0', 64);

		for (int8_t i = 0; token[i]; i++)
		{
			switch (token[i])
			{
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					while (token[i] > '0')
					{
						g_init_board.table[square++] = PIECE_NONE;
						token[i]--;
					}
					break ;

				case 'P':
					g_init_board.table[square++] = WHITE_PAWN;
					break ;

				case 'p':
					g_init_board.table[square++] = BLACK_PAWN;
					break ;

				case 'B':
					g_init_board.table[square++] = WHITE_BISHOP;
					break ;

				case 'b':
					g_init_board.table[square++] = BLACK_BISHOP;
					break ;

				case 'N':
					g_init_board.table[square++] = WHITE_KNIGHT;
					break ;

				case 'n':
					g_init_board.table[square++] = BLACK_KNIGHT;
					break ;

				case 'R':
					g_init_board.table[square++] = WHITE_ROOK;
					break ;

				case 'r':
					g_init_board.table[square++] = BLACK_ROOK;
					break ;

				case 'Q':
					g_init_board.table[square++] = WHITE_QUEEN;
					break ;

				case 'q':
					g_init_board.table[square++] = BLACK_QUEEN;
					break ;

				case 'K':
					g_init_board.table[square++] = WHITE_KING;
					break ;

				case 'k':
					g_init_board.table[square++] = BLACK_KING;
					break ;

				default:
					break ;
			}
		}

		for (int8_t sqr = 0; sqr < 32; sqr++)
		{
			int8_t	tmp;

			tmp = g_init_board.table[sqr];
			g_init_board.table[sqr] = g_init_board.table[vt_square(sqr)];
			g_init_board.table[vt_square(sqr)] = tmp;
		}

		token = strtok(NULL, " \t\n");

		if (*token == 'b')
			g_init_board.player = PLAYER_BLACK;

		token = strtok(NULL, " \t\n");

		for (int8_t castle = 0; token[castle]; castle++)
		{
			switch (token[castle])
			{
				case 'K':
					g_init_board.special_moves |= WHITE_OO;
					break ;

				case 'Q':
					g_init_board.special_moves |= WHITE_OOO;
					break ;

				case 'k':
					g_init_board.special_moves |= BLACK_OO;
					break ;

				case 'q':
					g_init_board.special_moves |= BLACK_OOO;
					break ;
			}
		}

		token = strtok(NULL, " \t\n");

		if (*token >= 'a' && *token <= 'h')
		{
			g_init_board.special_moves |= EN_PASSANT;
			g_init_board.special_moves |= (*token - 'a') << 4;
		}

		token = strtok(NULL, " \t\n");

		g_init_board.last_active_move = atoi(token);

		token = strtok(NULL, " \t\n");
	}
	else if (strcmp(token, "startpos") == 0)
	{
		g_init_board = (board_t){
			{
				WHITE_ROOK,		WHITE_KNIGHT,	WHITE_BISHOP,	WHITE_QUEEN,
				WHITE_KING,		WHITE_BISHOP,	WHITE_KNIGHT,	WHITE_ROOK,
				WHITE_PAWN,		WHITE_PAWN,		WHITE_PAWN,		WHITE_PAWN,
				WHITE_PAWN,		WHITE_PAWN,		WHITE_PAWN,		WHITE_PAWN,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				PIECE_NONE,		PIECE_NONE,		PIECE_NONE,		PIECE_NONE,
				BLACK_PAWN,		BLACK_PAWN,		BLACK_PAWN,		BLACK_PAWN,
				BLACK_PAWN,		BLACK_PAWN,		BLACK_PAWN,		BLACK_PAWN,
				BLACK_ROOK,		BLACK_KNIGHT,	BLACK_BISHOP,	BLACK_QUEEN,
				BLACK_KING,		BLACK_BISHOP,	BLACK_KNIGHT,	BLACK_ROOK
			},
			ANY_CASTLING, 0, PLAYER_WHITE, 0, 0
		};
	}

	if (g_inter_moves)
		free(g_inter_moves);

	g_inter_moves = movelist_init();

	memcpy(&g_real_board, &g_init_board, sizeof(board_t));
	token = strtok(NULL, " \t\n");

	if (token && strcmp(token, "moves") == 0)
	{
		token = strtok(NULL, " \t\n");

		while (token)
		{
			move_t	next = str_to_move(token);

			push_move(g_inter_moves, next);
			do_move(&g_real_board, next);

			token = strtok(NULL, " \t\n");
		}
	}

	free(copy);
}
