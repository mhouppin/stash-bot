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

#ifndef PAWNS_H
# define PAWNS_H

# include "board.h"

typedef struct pawn_entry_s
{
    hashkey_t key;
    bitboard_t attackSpan[COLOR_NB];
    bitboard_t attacks[COLOR_NB];
    bitboard_t attacks2[COLOR_NB];
    bitboard_t passed[COLOR_NB];
    scorepair_t value;
}
pawn_entry_t;

enum { PawnTableSize = 1 << 15 };

pawn_entry_t *pawn_probe(const board_t *board);

#endif
