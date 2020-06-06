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

#include "board.h"
#include "movelist.h"

typedef struct
{
	const board_t	*board;
	move_t			tt_move;
	move_t			killers[2];
	extmove_t		*current;
	extmove_t		*end;
	int				stage;
	extmove_t		moves[256];
}
selector_t;

enum	Stage
{
	MainTT,
	CaptureInit,
	GoodCapture,
	Killer,
	BadCapture,
	InitQuiet,
	Quiet,
	EvasionTT,
	EvasionInit,
	Evasion,
	QsearchTT,
	QcaptureInit,
	Qcapture
};

void	init_selector(selector_t *sel, const board_t *board, move_t tt_move,
		move_t *killers, bool in_qsearch);

move_t	next_move(selector_t *sel);
