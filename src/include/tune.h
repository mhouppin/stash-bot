/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TUNE_H
# define TUNE_H

# include <stddef.h>
# include "score.h"

typedef struct
{
	char	*name;
	int		*value;
	int		min;
	int		max;
	int		step;
	void	(*on_update)(void *);
}
tuning_int_t;

typedef struct
{
	char	*name;
	double	*value;
	double	min;
	double	max;
	double	step;
	void	(*on_update)(void *);
}
tuning_double_t;

typedef struct
{
	char	*name;
	score_t	*value;
	score_t	min;
	score_t	max;
	score_t	step;
	void	(*on_update)(void *);
}
tuning_score_t;

typedef struct
{
	char		*name;
	scorepair_t	*value;
	scorepair_t	min;
	scorepair_t	max;
	scorepair_t	step;
	void		(*on_update)(void *);
}
tuning_spair_t;

typedef struct
{
	char	*fen;
	double	svalue;
}		tuning_pos_t;

typedef struct
{
	tuning_int_t	*int_list;
	size_t			int_count;
	size_t			int_max_count;

	tuning_double_t	*double_list;
	size_t			double_count;
	size_t			double_max_count;

	tuning_score_t	*score_list;
	size_t			score_count;
	size_t			score_max_count;

	tuning_spair_t	*spair_list;
	size_t			spair_count;
	size_t			spair_max_count;

	tuning_pos_t	*pos_list;
	size_t			pos_count;
	size_t			pos_max_count;
}		tuning_session_t;

void		init_tuning_session(tuning_session_t *session);
void		add_tuning_positions(tuning_session_t *session,
			const char *filename);

void		add_tuning_int(tuning_session_t *session,
			const char *name, int *value, int min, int max,
			int step, void (*on_update)(void *));

void		add_tuning_double(tuning_session_t *session,
			const char *name, double *value, double min, double max,
			double step, void (*on_update)(void *));

void		add_tuning_score(tuning_session_t *session,
			const char *name, score_t *value, score_t min, score_t max,
			score_t step, void (*on_update)(void *));

void		add_tuning_scorepair(tuning_session_t *session,
			const char *name, scorepair_t *value, scorepair_t min, scorepair_t max,
			scorepair_t step, void (*on_update)(void *));

void		dump_item_values(tuning_session_t *session);
void		quit_tuning_session(tuning_session_t *session);

enum
{
	TS_Quiet,
	TS_Low,
	TS_Medium,
	TS_High
};

void		launch_session(tuning_session_t *session, int max_iterations,
			int thread_count, int ts_debug_level);

#endif
