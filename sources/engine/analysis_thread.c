/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   analysis_thread.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 03:55:19 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/01/29 07:49:14 by stash       ###    #+. /#+    ###.fr     */
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

/*
int16_t	evaluate(const board_t *board)
{
	int				p = 0;
	int16_t			endval = 0;
	int16_t			midval = 0;
	const int8_t	*table = board->table;

	for (int8_t i = 0; i < 64; ++i)
	{
		int8_t	piece = table[i];

		if (piece == PIECE_NONE)
			continue ;
		p++;
		if (piece >= BLACK_PAWN)
		{
			piece ^= 8;
			midval -= mpiece_score[piece];
			midval -= mtable_score[piece][i ^ SQ_A8];
			endval -= epiece_score[piece];
			endval -= etable_score[piece][i ^ SQ_A8];
		}
		else
		{
			midval += mpiece_score[piece];
			midval += mtable_score[piece][i];
			endval += epiece_score[piece];
			endval += etable_score[piece][i];
		}
	}
	if (board->special_moves & WHITE_CASTLING)
		midval += 100;
	if (board->special_moves & BLACK_CASTLING)
		midval -= 100;
	if (p <= 16)
		return (endval);
	else
		return ((endval * (32 - p) + midval * (p - 16)) / 16);
}
*/

int move_priority(const void *l, const void *r, void *b)
{
	const move_t	*lm = l;
	const move_t	*rm = r;
	const board_t	*board = b;

	return ((int)(board->table[move_to(*rm)] - board->table[move_to(*lm)]));
}

int16_t	_alpha_beta(board_t *board, int max_depth, int16_t alpha, int16_t beta,
		clock_t movetime, clock_t start, int cur_depth)
{
	int16_t		value;
	movelist_t	*moves;
	board_t		tmp;

	if (g_curnodes >= g_nodes)
		return (INT16_MIN);

	if (g_curnodes % 16384 == 0)
	{
		if (!g_infinite && chess_clock() - start > movetime)
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

		if (board->special_moves & WHITE_CASTLING)
			midval += 100;
		if (board->special_moves & BLACK_CASTLING)
			midval -= 100;
		if (board->pcount <= 16)
			return (board->escore);
		else
			return ((board->escore * (32 - board->pcount) + (board->mscore + midval) * (board->pcount - 16)) / 16);
	}

	moves = get_simple_moves(board);

	tmp = *board;

	if (moves->size == 0)
	{
		tmp.player ^= 1;
		if (is_checked(&tmp))
		{
			tmp.player ^= 1;
			return ((tmp.player == PLAYER_WHITE) ? -32000 + ((cur_depth + 1) / 2)
					: 32000 - ((cur_depth + 1) / 2));
		}
		else
			return (0);
	}

	if (max_depth > 1)
		qsort_r(moves->moves, moves->size, sizeof(move_t), &move_priority, board);

	if (tmp.player == PLAYER_WHITE)
	{
		value = INT16_MIN + 1;

		for (size_t i = 0; i < moves->size; i++)
		{
			tmp = *board;

			do_move(&tmp, moves->moves[i]);

			int16_t	next = _alpha_beta(&tmp, max_depth - 1, alpha, beta,
					movetime, start, cur_depth + 1);

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
					movetime, start, cur_depth + 1);

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

int16_t	alpha_beta(move_t move, clock_t start, int16_t alpha, int16_t beta)
{
	board_t		start_board = g_real_board;

	do_move(&start_board, move);

	char *str = move_to_str(move);
	printf("info depth %d nodes %zu currmove %s\n", g_curdepth, g_curnodes, str);
	fflush(stdout);
	free(str);

	return (_alpha_beta(&start_board, g_curdepth,
				alpha, beta,
				(g_mintime > g_movetime) ? g_mintime : g_movetime,
				start, 0));
}

void	*analysis_thread(void *tid)
{
	int16_t		alpha = -30000;
	int16_t		beta = 30000;

	for (size_t i = (size_t)*(int *)tid; i < g_searchmoves->size; i += g_threads)
	{
		pthread_mutex_lock(&mtx_engine);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			pthread_mutex_unlock(&mtx_engine);
			return (NULL);
		}
		pthread_mutex_unlock(&mtx_engine);

		int16_t	value = alpha_beta(g_searchmoves->moves[i], g_start, alpha, beta);
		g_valuemoves[i] = value;

		if (g_real_board.player == PLAYER_WHITE)
		{
			if (alpha < value)
				alpha = value;
		}
		else
		{
			if (beta > value)
				beta = value;
		}
	}
	return (NULL);
}
