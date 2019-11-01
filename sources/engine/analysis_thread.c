/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   analysis_thread.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 03:55:19 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/01 17:52:13 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>

const int16_t	table_score[64] = {
	0, 5, 10, 15, 15, 10, 5, 0,
	5, 15, 25, 35, 35, 25, 15, 5,
	10, 25, 40, 55, 55, 40, 25, 10,
	15, 35, 55, 75, 75, 55, 35, 15,
	25, 45, 65, 85, 85, 65, 45, 25,
	15, 30, 45, 60, 60, 45, 30, 15,
	10, 20, 30, 40, 40, 30, 20, 10,
	5, 10, 15, 20, 20, 15, 10, 5
};

const int16_t	pawn_score[64] = {
	0, 5, 10, 15, 15, 10, 5, 0,
	5, 15, 25, 35, 35, 25, 15, 5,
	10, 25, 40, 55, 55, 40, 25, 10,
	15, 35, 55, 75, 75, 55, 35, 15,
	25, 45, 65, 85, 85, 65, 45, 25,
	35, 60, 85, 110, 110, 85, 60, 35,
	45, 75, 105, 135, 135, 105, 75, 45,
	0, 0, 0, 0, 0, 0, 0, 0
};

int16_t	evaluate(board_t *board)
{
	int16_t	ret = 0;
	int16_t	kval = 0;
	int16_t	p = 0;

	for (int8_t i = 0; i < 64; i++)
	{
		switch (board->table[i])
		{
			case WHITE_PAWN:
				ret += 100;
				ret += pawn_score[i];
				p++;
				break ;

			case WHITE_BISHOP:
				ret += 330;
				ret += table_score[i];
				p++;
				break ;

			case WHITE_KNIGHT:
				ret += 300;
				ret += table_score[i];
				p++;
				break ;

			case WHITE_ROOK:
				ret += 500;
				ret += table_score[i];
				p++;
				break ;

			case WHITE_QUEEN:
				ret += 900;
				ret += table_score[i];
				p++;
				break ;

			case WHITE_KING:
				kval += table_score[i];
				p++;
				break ;

			case BLACK_PAWN:
				ret -= 100;
				ret -= pawn_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;

			case BLACK_BISHOP:
				ret -= 330;
				ret -= table_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;

			case BLACK_KNIGHT:
				ret -= 300;
				ret -= table_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;

			case BLACK_ROOK:
				ret -= 500;
				ret -= table_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;

			case BLACK_QUEEN:
				ret -= 900;
				ret -= table_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;

			case BLACK_KING:
				kval -= table_score[(i & 7) + (7 - (i >> 3)) * 8];
				p++;
				break ;
		}
	}
	if (p >= 16)
		return (ret - kval);
	else
		return (ret + kval);
}

int16_t	_alpha_beta(board_t *board, int max_depth, int16_t alpha, int16_t beta,
		size_t *max_nodes, clock_t movetime, clock_t start, int cur_depth)
{
	int16_t		value;
	movelist_t	*moves;
	board_t		tmp;

	if (*max_nodes == 0)
		return (INT16_MIN);

	(*max_nodes)--;

	if (cur_depth < 2)
	{
		if (!g_infinite && clock() - start > movetime)
			return (INT16_MIN);
		else
		{
			pthread_mutex_lock(&mtx_engine);

			if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
			{
				pthread_mutex_unlock(&mtx_engine);
				return (INT16_MIN);
			}
			pthread_mutex_unlock(&mtx_engine);
		}
	}

	tmp = *board;

	if (max_depth == 0)
	{
		return (evaluate(&tmp));
	}

	moves = get_simple_moves(board);

	if (moves->size == 0)
	{
		tmp.player ^= 1;
		if (is_checked(&tmp))
		{
			tmp.player ^= 1;
			return ((tmp.player == PLAYER_WHITE) ? -16000 + ((cur_depth + 1) / 2)
					: 16000 - ((cur_depth + 1) / 2));
		}
		else
			return (0);
	}

	if (tmp.player == PLAYER_WHITE)
	{
		value = INT16_MIN + 1;

		for (size_t i = 0; i < moves->size; i++)
		{
			tmp = *board;

			do_move(&tmp, moves->moves[i]);

			int16_t	next = _alpha_beta(&tmp, max_depth - 1, alpha, beta,
					max_nodes, movetime, start, cur_depth + 1);

			if (next == INT16_MIN)
			{
				value = INT16_MIN;
				break ;
			}

			if (value < next)
				value = next;

			if (alpha < value)
			{
				if (alpha >= beta)
					break ;
				alpha = value;
			}
		}
	}
	else
	{
		value = INT16_MAX - 1;

		for (size_t i = 0; i < moves->size; i++)
		{
			tmp = *board;

			do_move(&tmp, moves->moves[i]);

			int16_t next = _alpha_beta(&tmp, max_depth - 1, alpha, beta,
					max_nodes, movetime, start, cur_depth + 1);

			if (next == INT16_MIN)
			{
				value = INT16_MIN;
				break ;
			}

			if (value > next)
				value = next;

			if (beta > value)
			{
				if (beta <= alpha)
					break ;
				beta = value;
			}
		}
	}

	movelist_quit(moves);
	return (value);
}

int16_t	alpha_beta(move_t move, clock_t start, size_t *max_nodes)
{
	board_t		start_board = g_real_board;

	do_move(&start_board, move);

	char *str = move_to_str(move);
	printf("info depth %d nodes %zu currmove %s\n", g_curdepth,
			(g_nodes / g_threads) - *max_nodes, str);
	free(str);

	return (_alpha_beta(&start_board, g_curdepth,
				INT16_MIN + 1, INT16_MAX - 1,
				max_nodes, (g_mintime > g_movetime) ? g_mintime : g_movetime,
				start, 0));
}

void	*analysis_thread(void *tid)
{
	size_t		max_nodes = g_nodes / g_threads;

	for (size_t i = (size_t)*(int *)tid; i < g_searchmoves->size; i += g_threads)
	{
		pthread_mutex_lock(&mtx_engine);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			pthread_mutex_unlock(&mtx_engine);
			return (NULL);
		}
		pthread_mutex_unlock(&mtx_engine);

		int16_t	value = alpha_beta(g_searchmoves->moves[i], g_start, &max_nodes);

		if (value != INT16_MIN)
			g_valuemoves[i] = value;
	}
	return (NULL);
}
