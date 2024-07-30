/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include "new_hashkey.h"

#include "new_chess_types.h"
#include "new_random.h"

Key ZobristPsq[PIECE_NB][SQUARE_NB];
Key ZobristEnPassant[FILE_NB];
Key ZobristCastling[CASTLING_NB];
Key ZobristSideToMove;

void zobrist_init(void) {
    u64 seed = 0x7F6E5D4C3B2A1908ull;

    for (Piece piece = WHITE_PAWN; piece <= BLACK_KING; ++piece) {
        for (Square square = SQ_A1; square <= SQ_H8; ++square) {
            ZobristPsq[piece][square] = u64_random(&seed);
        }
    }

    for (File file = FILE_A; file <= FILE_H; ++file) {
        ZobristEnPassant[file] = u64_random(&seed);
    }

    for (CastlingRights cr = 0; cr < CASTLING_NB; ++cr) {
        ZobristCastling[cr] = 0;
        u64 b = cr;

        while (b) {
            Key key = ZobristCastling[U64(1) << u64_first_one(b)];
            ZobristCastling[cr] ^= key ?: u64_random(&seed);
            b &= b - 1;
        }
    }

    ZobristSideToMove = u64_random(&seed);
}
