/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci.h                                            .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 18:18:59 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:41:22 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef UCI_H
# define UCI_H

# include <pthread.h>
# include <stddef.h>
# include <time.h>
# include "inlining.h"

enum	e_egn_mode
{
	WAITING,
	THINKING
};

enum	e_egn_send
{
	DO_NOTHING,
	DO_THINK,
	DO_EXIT,
	DO_ABORT
};

typedef struct	goparams_s
{
	clock_t		wtime;
	clock_t		btime;
	clock_t		winc;
	clock_t		binc;
	int			movestogo;
	int			depth;
	size_t		nodes;
	int			mate;
	int			infinite;
	int			perft;
	clock_t		movetime;
}				goparams_t;

typedef struct	ucioptions_s
{
	clock_t		move_overhead;
	int			multi_pv;
	clock_t		min_think_time;
	int			chess960;
}				ucioptions_t;

extern pthread_mutex_t	g_engine_mutex;
extern pthread_cond_t	g_engine_condvar;
extern enum e_egn_mode	g_engine_mode;
extern enum e_egn_send	g_engine_send;
extern goparams_t		g_goparams;

typedef struct	cmdlink_s
{
	const char	*cmd_name;
	void		(*call)(const char *);
}				cmdlink_t;

INLINED clock_t	chess_clock(void)
{
	struct timespec	tp;

	clock_gettime(CLOCK_REALTIME, &tp);
	return ((clock_t)tp.tv_sec * 1000 +
			(clock_t)tp.tv_nsec / 1000000);
}

void	uci_bench(const char *args);
void	uci_d(const char *args);
void	uci_debug(const char *args);
void	uci_go(const char *args);
void	uci_isready(const char *args);
void	uci_position(const char *args);
void	uci_quit(const char *args);
void	uci_setoption(const char *args);
void	uci_stop(const char *args);
void	uci_uci(const char *args);
void	uci_ucinewgame(const char *args);

#endif
