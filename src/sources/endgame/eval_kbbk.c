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

#include "endgame.h"

score_t	eval_kbbk(const board_t *board)
{
	{
		bitboard_t	dsb = board->piecetype_bits[BISHOP] & DARK_SQUARES;
		if (!dsb || more_than_one(dsb))
			return (0);
	}

	score_t	base_score = VICTORY + KBBK_Bonus;

	square_t	strong_k = board->piece_list[WHITE_KING][0];
	square_t	weak_k = board->piece_list[BLACK_KING][0];

	if (board->piece_count[BLACK_BISHOP])
	{
		square_t	tmp = strong_k;
		strong_k = weak_k;
		weak_k = tmp;
	}

	base_score += proximity_bonus(strong_k, weak_k) + cornered_bonus(weak_k);

	return (more_than_one(board->color_bits[board->side_to_move]) ? base_score : -base_score);
}
