/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#ifndef MOVE_H
# define MOVE_H

# include <stdint.h>
# include "inlining.h"
# include "piece.h"
# include "square.h"

typedef int32_t move_t;
typedef int32_t movetype_t;

enum
{
    NO_MOVE = 0,
    NULL_MOVE = 65,

    NORMAL_MOVE = 0,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING = 3 << 14,
    MOVETYPE_MASK = 3 << 14
};

INLINED square_t from_sq(move_t move)
{
    return ((square_t)((move >> 6) & SQ_H8));
}

INLINED square_t to_sq(move_t move)
{
    return ((square_t)(move & SQ_H8));
}

INLINED int square_mask(move_t move)
{
    return (move & 0xFFF);
}

INLINED movetype_t move_type(move_t move)
{
    return (move & MOVETYPE_MASK);
}

INLINED piecetype_t promotion_type(move_t move)
{
    return ((piecetype_t)(((move >> 12) & 3) + KNIGHT));
}

INLINED move_t create_move(square_t from, square_t to)
{
    return ((move_t)((from << 6) + to));
}

INLINED move_t reverse_move(move_t move)
{
    return (create_move(to_sq(move), from_sq(move)));
}

INLINED move_t create_promotion(square_t from, square_t to, piecetype_t piecetype)
{
    return ((move_t)(PROMOTION + ((piecetype - KNIGHT) << 12) + (from << 6) + to));
}

INLINED move_t create_en_passant(square_t from, square_t to)
{
    return ((move_t)(EN_PASSANT + (from << 6) + to));
}

INLINED move_t create_castling(square_t from, square_t to)
{
    return ((move_t)(CASTLING + (from << 6) + to));
}

INLINED bool is_valid_move(move_t move)
{
    return (from_sq(move) != to_sq(move));
}

#endif
