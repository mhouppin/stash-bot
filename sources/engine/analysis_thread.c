/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   analysis_thread.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 03:55:19 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/13 16:11:20 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>

const int16_t	mpiece_score[8] = {
	0, 100, 300, 330, 500, 900, 0, 0,
};

const int16_t	epiece_score[8] = {
	0, 200, 600, 660, 1000, 1800, 0, 0
};

const int16_t	mtable_score[8][64] = {
	{0},
	{
		0,		0,		0,		0,		0,		0,		0,		0,
		0,		0,		0,		-10,	-15,	0,		0,		0,
		5,		10,		5,		10,		15,		10,		10,		5,
		5,		5,		10,		30,		40,		15,		5,		5,
		10,		10,		10,		35,		45,		15,		10,		10,
		20,		15,		20,		40,		45,		25,		15,		20,
		35,		30,		35,		45,		50,		35,		30,		35,
		0,		0,		0,		0,		0,		0,		0,		0
	},
	{
		-60,	-20,	-10,	0,		0,		-10,	-20,	-60,
		-45,	-25,	10,		15,		15,		10,		-25,	-45,
		-5,		15,		20,		20,		20,		20,		15,		-5,
		5,		20,		25,		30,		30,		25,		20,		5,
		0,		10,		20,		35,		35,		20,		10,		0,
		10,		15,		20,		40,		40,		20,		15,		10,
		-5,		5,		10,		25,		25,		10,		5,		-5,
		-30,	-10,	-5,		0,		0,		-5,		-10,	-30
	},
	{
		-55,	-5,		-10,	-25,	-25,	-10,	-5,		-55,
		-15,	10,		20,		5,		5,		20,		10,		-15,
		-5,		20,		-5,		15,		15,		-5,		20,		-5,
		-5,		10,		25,		40,		40,		25,		10,		-5,
		-10,	30,		20,		30,		30,		20,		30,		-10,
		-15,	5,		0,		10,		10,		0,		5,		-15,
		-15,	-15,	5,		0,		0,		5,		-15,	-15,
		-50,	0,		-15,	-25,	-25,	-15,	0,		-50
	},
	{
		-30,	-20,	-15,	-5,		-5,		-15,	-20,	-30,
		-20,	-15,	-10,	5,		5,		-10,	-15,	-20,
		-25,	-10,	0,		5,		5,		0,		-10,	-25,
		-15,	-5,		-5,		-5,		-5,		-5,		-5,		-15,
		-25,	-15,	-5,		5,		5,		-5,		-15,	-25,
		-20,	0,		5,		10,		10,		5,		0,		-20,
		0,		10,		15,		20,		20,		15,		10,		0,
		-15,	-20,	0,		10,		10,		0,		-20,	-15
	},
	{
		5,		-5,		-5,		5,		5,		-5,		-5,		5,
		-5,		5,		10,		10,		10,		10,		5,		-5,
		-5,		5,		15,		5,		5,		15,		5,		-5,
		5,		5,		10,		10,		10,		10,		5,		5,
		0,		15,		10,		5,		5,		10,		15,		0,
		-5,		10,		5,		10,		10,		5,		10,		-5,
		-5,		5,		10,		10,		10,		10,		5,		-5,
		0,		0,		0,		0,		0,		0,		0,		0
	},
	{
		270,	325,	270,	190,	190,	270,	325,	270,
		280,	300,	230,	175,	175,	230,	300,	280,
		195,	260,	170,	120,	120,	170,	260,	195,
		165,	190,	140,	100,	100,	140,	190,	165,
		155,	180,	105,	70,		70,		105,	180,	155,
		125,	145,	80,		30,		30,		80,		145,	125,
		90,		120,	65,		35,		35,		65,		120,	90,
		60,		90,		45,		0,		0,		45,		90,		60
	},
	{0}
};

const int16_t	etable_score[8][64] = {
	{0},
	{
		0,		0,		0,		0,		0,		0,		0,		0,
		-10,	-5,		10,		0,		15,		5,		-5,		-20,
		-10,	-10,	-10,	5,		5,		5,		-5,		-5,
		5,		0,		-10,	-5,		-15,	-10,	-10,	-10,
		10,		5,		5,		-10,	-10,	-5,		15,		10,
		30,		20,		20,		30,		30,		5,		5,		15,
		0,		-10,	10,		20,		25,		20,		5,		5,
		0,		0,		0,		0,		0,		0,		0,		0
	},
	{
		-95,	-65,	-50,	-20,	-20,	-50,	-65,	-95,
		-65,	-55,	-20,	10,		10,		-20,	-55,	-65,
		-40,	-25,	-10,	30,		30,		-10,	-25,	-40,
		-35,	0,		15,		30,		30,		15,		0,		-35,
		-45,	-15,	10,		40,		40,		10,		-15,	-45,
		-50,	-45,	-15,	15,		15,		-15,	-45,	-50,
		-70,	-50,	-50,	10,		10,		-50,	-50,	-70,
		-100,	-90,	-55,	-15,	-15,	-55,	-90,	-100
	},
	{
		-55,	-30,	-35,	-10,	-10,	-35,	-30,	-55,
		-35,	-15,	-15,	0,		0,		-15,	-15,	-35,
		-15,	0,		0,		10,		10,		0,		0,		-15,
		-20,	-5,		0,		15,		15,		0,		-5,		-20,
		-15,	0,		-15,	15,		15,		-15,	0,		-15,
		-30,	5,		5,		5,		5,		5,		5,		-30,
		-30,	-20,	0,		0,		0,		0,		-20,	-30,
		-45,	-40,	-35,	-25,	-25,	-35,	-40,	-45
	},
	{
		-10,	-15,	-10,	-10,	-10,	-10,	-15,	-10,
		-10,	-10,	0,		0,		0,		0,		-10,	-10,
		5,		-10,	0,		-5,		-5,		0,		-10,	5,
		-5,		0,		-10,	5,		5,		-10,	0,		-5,
		-5,		10,		5,		-5,		-5,		5,		10,		-5,
		5,		0,		-5,		10,		10,		-5,		0,		5,
		5,		5,		20,		-5,		-5,		20,		5,		5,
		20,		0,		20,		15,		15,		20,		0,		20
	},
	{
		-70,	-55,	-45,	-25,	-25,	-45,	-55,	-70,
		-55,	-30,	-20,	-5,		-5,		-20,	-30,	-55,
		-40,	-20,	-10,	5,		5,		-10,	-20,	-40,
		-25,	-5,		15,		25,		25,		15,		-5,		-25,
		-30,	-5,		10,		20,		20,		10,		-5,		-30,
		-40,	-20,	-10,	0,		0,		-10,	-20,	-40,
		-50,	-25,	-25,	-10,	-10,	-25,	-25,	-50,
		-75,	-50,	-45,	-35,	-35,	-45,	-50,	-75
	},
	{
		0,		45,		85,		75,		75,		85,		45,		0,
		55,		100,	135,	135,	135,	135,	100,	55,
		90,		130,	170,	175,	175,	170,	130,	90,
		105,	155,	170,	170,	170,	170,	155,	105,
		95,		165,	200,	200,	200,	200,	165,	95,
		90,		170,	185,	190,	190,	185,	170,	90,
		45,		120,	115,	130,	130,	115,	120,	45,
		10,		60,		75,		80,		80,		75,		60,		10
	},
	{0}
};

void	order_moves(movelist_t *moves, const board_t *board)
{
	size_t	gap, start;
	ssize_t	i;

	for (gap = moves->size / 2; gap > 0; gap /= 2)
	{
		for (start = gap; start < moves->size; ++start)
		{
			for (i = (ssize_t)(start - gap); i >= 0; i = i - gap)
			{
				int		value =
					board->table[move_to(moves->moves[i + gap])] -
					board->table[move_to(moves->moves[i])];

				if (value <= 0)
					break ;
				else
				{
					move_t	tmp;

					tmp = moves->moves[i + gap];
					moves->moves[i + gap] = moves->moves[i];
					moves->moves[i] = tmp;
				}
			}
		}
	}
}

int move_priority(const void *l, const void *r, void *b)
{
	const move_t	*lm = l;
	const move_t	*rm = r;
	const board_t	*board = b;

	return ((int)(board->table[move_to(*rm)] - board->table[move_to(*lm)]));
}

int16_t	alpha_beta(board_t *board, int max_depth, int16_t alpha, int16_t beta,
		clock_t end, int cur_depth)
{
	movelist_t	*moves;
	board_t		tmp;

	if (g_curnodes >= g_nodes)
		return (INT16_MIN);

	if (g_curnodes % 16384 == 0)
	{
		if (!g_infinite && chess_clock() > end)
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

	g_curnodes++;

	if (max_depth == 0)
	{
		int		midval = 0;
		int		score;

		if (board->special_moves & WHITE_CASTLING)
			midval += 100;
		if (board->special_moves & BLACK_CASTLING)
			midval -= 100;
		if (board->pcount <= 16)
			score = board->escore;
		else
			score = ((board->escore * (32 - board->pcount) + (board->mscore + midval) * (board->pcount - 16)) / 16);

		return (board->player == PLAYER_WHITE ? score : -score);
	}

	moves = get_simple_moves(board);

	tmp = *board;

	if (moves->size == 0)
	{
		movelist_quit(moves);
		tmp.player ^= 1;
		if (is_checked(&tmp))
			return (-32000 + ((cur_depth + 1) / 2));
		else
			return (0);
	}

	if (max_depth > 1)
		order_moves(moves, board);

	for (size_t i = 0; i < moves->size; i++)
	{
		tmp = *board;

		do_move(&tmp, moves->moves[i]);

		int16_t next;
		
		if (i == 0)
			next = -alpha_beta(&tmp, max_depth - 1, -beta, -alpha,
				end, cur_depth + 1);
		else
		{
			next = -alpha_beta(&tmp, max_depth - 1, -alpha - 1,
				-alpha, end, cur_depth + 1);

			if (alpha < next && next < beta)
				next = -alpha_beta(&tmp, max_depth - 1, -beta,
					-next, end, cur_depth + 1);
		}

		if (next == INT16_MIN)
		{
			alpha = INT16_MIN;
			break ;
		}

		if (alpha < next)
			alpha = next;

		if (alpha >= beta)
			break ;
	}

	movelist_quit(moves);
	return (alpha);
}

void	*analysis_thread(void *tid)
{
	int16_t		alpha = -30000;

	for (size_t i = (size_t)*(int *)tid; i < g_searchmoves->size; i += g_threads)
	{
		pthread_mutex_lock(&mtx_engine);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			pthread_mutex_unlock(&mtx_engine);
			return (NULL);
		}
		pthread_mutex_unlock(&mtx_engine);

		board_t		start_board = g_real_board;

		do_move(&start_board, g_searchmoves->moves[i]);

		if (chess_clock() - g_start > 3000)
		{
			char	*str = move_to_str(g_searchmoves->moves[i]);
			printf("info depth %d nodes %zu currmove %s currmovenumber %zu\n",
					g_curdepth + 1, g_curnodes, str, i + 1);
			fflush(stdout);
			free(str);
		}

		clock_t		end = g_start + (g_mintime > g_movetime ? g_mintime : g_movetime);

		if (i == (size_t)*(int *)tid)
			g_valuemoves[i] = -alpha_beta(&start_board, g_curdepth, -30000, -alpha, end, 0);
		else
		{
			g_valuemoves[i] = -alpha_beta(&start_board, g_curdepth, -alpha - 1, -alpha, end, 0);

			if (alpha < g_valuemoves[i])
				g_valuemoves[i] = -alpha_beta(&start_board, g_curdepth, -30000, -g_valuemoves[i], end, 0);
		}

		if (g_valuemoves[i] > alpha)
			alpha = g_valuemoves[i];
		else if (g_valuemoves[i] == alpha)
			g_valuemoves[i] = alpha - 1;
	}
	return (NULL);
}
