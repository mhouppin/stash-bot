#ifndef ATTACKS_H
#define ATTACKS_H

#include "new_bitboard.h"
#include "new_chess_types.h"
#include "new_core.h"

INLINED Bitboard pawn_attacks_bb(Square square, Color color)
{
    HINT(square_is_valid(square));
    HINT(color_is_valid(color));
    extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

    return PawnAttacks[color][square];
}

INLINED Bitboard knight_attacks_bb(Square square)
{
    HINT(square_is_valid(square));
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];

    return RawAttacks[KNIGHT][square];
}

INLINED Bitboard king_attacks_bb(Square square)
{
    HINT(square_is_valid(square));
    extern Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];

    return RawAttacks[KING][square];
}

INLINED Bitboard bishop_attacks_bb(Square square, Bitboard occupancy)
{
    HINT(square_is_valid(square));
    extern Magic BishopMagics[SQUARE_NB];
    const Magic *magic = &BishopMagics[square];

    return magic->moves[magic_index(magic, occupancy)];
}

INLINED Bitboard rook_attacks_bb(Square square, Bitboard occupancy)
{
    HINT(square_is_valid(square));
    extern Magic RookMagics[SQUARE_NB];
    const Magic *magic = &RookMagics[square];

    return magic->moves[magic_index(magic, occupancy)];
}

INLINED Bitboard queen_attacks_bb(Square square, Bitboard occupancy)
{
    HINT(square_is_valid(square));
    return bishop_attacks_bb(square, occupancy) | rook_attacks_bb(square, occupancy);
}

INLINED Bitboard attacks_bb(Piecetype piecetype, Square square, Bitboard occupancy)
{
    HINT(square_is_valid(square));
    HINT(piecetype >= KNIGHT && piecetype <= KING);

    switch (piecetype)
    {
        case KNIGHT: return knight_attacks_bb(square);
        case BISHOP: return bishop_attacks_bb(square, occupancy);
        case ROOK: return rook_attacks_bb(square, occupancy);
        case QUEEN: return queen_attacks_bb(square, occupancy);
        case KING: return king_attacks_bb(square);
    }

    return 0;
}

#endif
