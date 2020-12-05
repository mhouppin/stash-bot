/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "engine.h"
#include "lazy_smp.h"
#include "movelist.h"

bool    is_kxk(const board_t *board, color_t c)
{
    // Losing side must not have pieces or pawns left
    if (more_than_one(board->color_bits[not_color(c)]))
        return (false);

    // Do we have at least a major piece ?
    if (piecetypes_bb(board, ROOK, QUEEN))
        return (true);

    bitboard_t  bishops = piecetype_bb(board, BISHOP);
    bitboard_t  dsq = DARK_SQUARES;

    // Do we have two opposite colored bishops ?
    if ((bishops & dsq) && (bishops & ~dsq))
        return (true);

    // Do we have three minors ? (same colored bishops count only as one)
    if (popcount(piecetype_bb(board, KNIGHT)) + !!bishops >= 3)
        return (true);

    // In other cases use classical eval and/or specialized endgames (like KPK)
    return (false);
}

score_t eval_kxk(const board_t *board, color_t c)
{
    get_worker(board)->tb_hits++;

    // Check for no stalemates
    if (board->side_to_move != c && !board->stack->checkers)
    {
        movelist_t  list;

        list_all(&list, board);
        if (movelist_size(&list) == 0)
            return (0);
    }

    score_t score = endgame_score(board->psq_scorepair)
        + (c == WHITE ? VICTORY : -VICTORY);

    // Add a small Initiative bonus to the side to move
    return (15 + (board->side_to_move == WHITE ? score : -score));
}
