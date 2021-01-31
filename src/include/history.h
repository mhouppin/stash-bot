/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#ifndef HISTORY_H
# define HISTORY_H

# include <stdlib.h>
# include "move.h"
# include "piece.h"
# include "score.h"

enum
{
    HistoryMaxScore = 1024,
    HistoryScale = 16,
    HistoryResolution = HistoryMaxScore * HistoryScale
};

typedef int32_t bf_history_t[PIECE_NB][SQUARE_NB * SQUARE_NB];
typedef int32_t ct_history_t[PIECE_NB][SQUARE_NB][PIECETYPE_NB][SQUARE_NB];
typedef move_t  cm_history_t[PIECE_NB][SQUARE_NB];

INLINED void    add_bf_history(bf_history_t hist, piece_t piece, move_t move, int32_t bonus)
{
    int32_t *entry = &hist[piece][square_mask(move)];

    *entry += bonus - *entry * abs(bonus) / HistoryResolution;
}

INLINED score_t get_bf_history_score(bf_history_t hist, piece_t piece, move_t move)
{
    return (hist[piece][square_mask(move)] / HistoryScale);
}

INLINED void    add_ct_history(ct_history_t hist, piece_t pc, square_t to,
                piece_t lpc, square_t lto, int32_t bonus)
{
    int32_t *entry = &hist[pc][to][piece_type(lpc)][lto];

    *entry += bonus - *entry * abs(bonus) / HistoryResolution;
}

INLINED score_t get_ct_history_score(ct_history_t hist, piece_t pc, square_t to,
                piece_t lpc, square_t lto)
{
    return (hist[pc][to][piece_type(lpc)][lto] / HistoryScale);
}

#endif
