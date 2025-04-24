/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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

#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitboard.h"
#include "chess_types.h"
#include "core.h"

INLINED Bitboard pawn_attacks_bb(Square square, Color color) {
    assert(square_is_valid(square));
    assert(color_is_valid(color));
    extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

    return PawnAttacks[color][square];
}

INLINED Bitboard white_pawns_attacks_bb(Bitboard bb) {
    return bb_shift_up_left(bb) | bb_shift_up_right(bb);
}

INLINED Bitboard black_pawns_attacks_bb(Bitboard bb) {
    return bb_shift_down_left(bb) | bb_shift_down_right(bb);
}

INLINED Bitboard pawns_attacks_bb(Bitboard bb, Color color) {
    assert(color_is_valid(color));
    return color == WHITE ? white_pawns_attacks_bb(bb) : black_pawns_attacks_bb(bb);
}

INLINED Bitboard white_pawns_attacks2_bb(Bitboard bb) {
    return bb_shift_up_left(bb) & bb_shift_up_right(bb);
}

INLINED Bitboard black_pawns_attacks2_bb(Bitboard bb) {
    return bb_shift_down_left(bb) & bb_shift_down_right(bb);
}

INLINED Bitboard pawns_attacks2_bb(Bitboard bb, Color color) {
    assert(color_is_valid(color));
    return color == WHITE ? white_pawns_attacks2_bb(bb) : black_pawns_attacks2_bb(bb);
}

INLINED Bitboard knight_attacks_bb(Square square) {
    assert(square_is_valid(square));
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];

    return RawAttacks[KNIGHT][square];
}

INLINED Bitboard king_attacks_bb(Square square) {
    assert(square_is_valid(square));
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];

    return RawAttacks[KING][square];
}

INLINED Bitboard bishop_attacks_bb(Square square, Bitboard occupancy) {
    assert(square_is_valid(square));
    extern Magic BishopMagics[SQUARE_NB];
    const Magic *magic = &BishopMagics[square];

    return magic->moves[magic_index(magic, occupancy)];
}

INLINED Bitboard rook_attacks_bb(Square square, Bitboard occupancy) {
    assert(square_is_valid(square));
    extern Magic RookMagics[SQUARE_NB];
    const Magic *magic = &RookMagics[square];

    return magic->moves[magic_index(magic, occupancy)];
}

INLINED Bitboard queen_attacks_bb(Square square, Bitboard occupancy) {
    assert(square_is_valid(square));
    return bishop_attacks_bb(square, occupancy) | rook_attacks_bb(square, occupancy);
}

INLINED Bitboard attacks_bb(Piecetype piecetype, Square square, Bitboard occupancy) {
    assert(square_is_valid(square));
    assert(piecetype >= KNIGHT && piecetype <= KING);

    switch (piecetype) {
        case KNIGHT: return knight_attacks_bb(square);
        case BISHOP: return bishop_attacks_bb(square, occupancy);
        case ROOK: return rook_attacks_bb(square, occupancy);
        case QUEEN: return queen_attacks_bb(square, occupancy);
        case KING: return king_attacks_bb(square);
    }

    return 0;
}

INLINED Bitboard bishop_raw_attacks_bb(Square square) {
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];
    return RawAttacks[BISHOP][square];
}

INLINED Bitboard rook_raw_attacks_bb(Square square) {
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];
    return RawAttacks[ROOK][square];
}

#endif
