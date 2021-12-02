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

#include <stdlib.h>
#include "bitboard.h"
#include "imath.h"
#include "random.h"

bitboard_t LineBits[SQUARE_NB][SQUARE_NB];
bitboard_t PseudoMoves[PIECETYPE_NB][SQUARE_NB];
bitboard_t PawnMoves[COLOR_NB][SQUARE_NB];
magic_t BishopMagics[SQUARE_NB];
magic_t RookMagics[SQUARE_NB];
int SquareDistance[SQUARE_NB][SQUARE_NB];

bitboard_t HiddenRookTable[0x19000];
bitboard_t HiddenBishopTable[0x1480];

// Returns a bitboard representing all the reachable squares by a bishop
// (or rook) from given square and given occupied squares.

bitboard_t sliding_attack(const direction_t *directions, square_t square, bitboard_t occupied)
{
    bitboard_t attack = 0;

    for (int i = 0; i < 4; ++i)
        for (square_t s = square + directions[i];
            is_valid_sq(s) && SquareDistance[s][s - directions[i]] == 1; s += directions[i])
        {
            attack |= square_bb(s);
            if (occupied & square_bb(s))
                break ;
        }

    return (attack);
}

// Initializes magic bitboard tables necessary for bishop, rook and queen moves.

void magic_init(bitboard_t *table, magic_t *magics, const direction_t *directions)
{
    bitboard_t reference[4096], edges, b;

#ifndef USE_PEXT
    bitboard_t occupancy[4096];
    int epoch[4096] = {0}, counter = 0;
#endif

    int size = 0;

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        edges = ((RANK_1_BITS | RANK_8_BITS) & ~sq_rank_bb(square))
            | ((FILE_A_BITS | FILE_H_BITS) & ~sq_file_bb(square));

        magic_t *m = magics + square;

        m->mask = sliding_attack(directions, square, 0) & ~edges;
        m->shift = 64 - popcount(m->mask);
        m->moves = (square == SQ_A1) ? table : magics[square - 1].moves + size;

        b = 0;
        size = 0;

        do {
#ifndef USE_PEXT
            occupancy[size] = b;
#endif
            reference[size] = sliding_attack(directions, square, b);

#ifdef USE_PEXT
            m->moves[_pext_u64(b, m->mask)] = reference[size];
#endif
            size++;
            b = (b - m->mask) & m->mask;
        }
        while (b);

#ifndef USE_PEXT

        qseed(4);

        for (int i = 0; i < size; )
        {
            for (m->magic = 0; popcount((m->magic * m->mask) >> 56) < 6; )
                m->magic = qrandom() & qrandom() & qrandom();

            for (++counter, i = 0; i < size; ++i)
            {
                unsigned int index = magic_index(m, occupancy[i]);

                if (epoch[index] < counter)
                {
                    epoch[index] = counter;
                    m->moves[index] = reference[i];
                }
                else if (m->moves[index] != reference[i])
                    break ;
            }
        }
#endif
    }
}

// Initializes all bitboard tables at program startup.

void bitboard_init(void)
{
    static const direction_t kingDirections[8] = {
        -9, -8, -7, -1, 1, 7, 8, 9
    };
    static const direction_t knightDirections[8] = {
        -17, -15, -10, -6, 6, 10, 15, 17
    };
    static const direction_t bishopDirections[4] = {
        -9, -7, 7, 9
    };
    static const direction_t rookDirections[4] = {
        -8, -1, 1, 8
    };

    // Initializes square distance table.

    for (square_t sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (square_t sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            int fileDistance = abs(sq_file(sq1) - sq_file(sq2));
            int rankDistance = abs(sq_rank(sq1) - sq_rank(sq2));

            SquareDistance[sq1][sq2] = max(fileDistance, rankDistance);
        }

    // Initializes pawn pseudo-moves table.

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        bitboard_t b = square_bb(square);

        PawnMoves[WHITE][square] = (shift_up_left(b) | shift_up_right(b));
        PawnMoves[BLACK][square] = (shift_down_left(b) | shift_down_right(b));
    }

    // Initializes king and knight pseudo-moves table.

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        for (int i = 0; i < 8; ++i)
        {
            square_t to = square + kingDirections[i];

            if (is_valid_sq(to) && SquareDistance[square][to] <= 2)
                PseudoMoves[KING][square] |= square_bb(to);
        }

        for (int i = 0; i < 8; ++i)
        {
            square_t to = square + knightDirections[i];

            if (is_valid_sq(to) && SquareDistance[square][to] <= 2)
                PseudoMoves[KNIGHT][square] |= square_bb(to);
        }
    }

    magic_init(HiddenBishopTable, BishopMagics, bishopDirections);
    magic_init(HiddenRookTable, RookMagics, rookDirections);

    // Initializes bishop, rook and queen pseudo-moves table.

    for (square_t sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
    {
        PseudoMoves[QUEEN][sq1] = PseudoMoves[BISHOP][sq1] = bishop_moves_bb(sq1, 0);
        PseudoMoves[QUEEN][sq1] |= PseudoMoves[ROOK][sq1] = rook_moves_bb(sq1, 0);

        for (square_t sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            if (PseudoMoves[BISHOP][sq1] & square_bb(sq2))
                LineBits[sq1][sq2] = (bishop_moves_bb(sq1, 0) & bishop_moves_bb(sq2, 0))
                    | square_bb(sq1) | square_bb(sq2);

            if (PseudoMoves[ROOK][sq1] & square_bb(sq2))
                LineBits[sq1][sq2] = (rook_moves_bb(sq1, 0) & rook_moves_bb(sq2, 0))
                    | square_bb(sq1) | square_bb(sq2);
        }
    }
}
