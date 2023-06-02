/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#include "bitboard.h"
#include "random.h"
#include "types.h"
#include <stdlib.h>

bitboard_t LineBB[SQUARE_NB][SQUARE_NB];
bitboard_t PseudoMoves[PIECETYPE_NB][SQUARE_NB];
bitboard_t PawnMoves[COLOR_NB][SQUARE_NB];
Magic BishopMagics[SQUARE_NB];
Magic RookMagics[SQUARE_NB];
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
            if (occupied & square_bb(s)) break;
        }

    return attack;
}

// Initializes magic bitboard tables necessary for bishop, rook and queen moves.
void magic_init(bitboard_t *table, Magic *magics, const direction_t *directions)
{
    bitboard_t reference[4096], edges, b;

#ifndef USE_PEXT
    bitboard_t occupancy[4096];

    // The epoch is used to determine which iteration of the occupancy test
    // we are in, to avoid having to zero the attack array between each failed
    // iteration.
    int epoch[4096] = {0}, currentEpoch = 0;
    uint64_t seed = 64;
#endif

    int size = 0;

    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        // The edges of the board are not counted in the occupancy bits
        // (since we can reach them whether there's a piece on them or not
        // because of capture moves), but we must still ensure they're
        // accounted for if the piece is already on them for Rook moves.
        edges = ((RANK_1_BB | RANK_8_BB) & ~sq_rank_bb(square))
                | ((FILE_A_BB | FILE_H_BB) & ~sq_file_bb(square));

        Magic *m = magics + square;

        // Compute the occupancy for the given square, excluding edges as
        // explained before.
        m->mask = sliding_attack(directions, square, 0) & ~edges;

        // We will need popcount(mask) bits of information for indexing
        // the occupancy, and (1 << popcount(mask)) entries in the table
        // for storing the corresponding attack bitboards.
        m->shift = 64 - popcount(m->mask);

        // We use the entry count of the previous square for computing the
        // next index.
        m->moves = (square == SQ_A1) ? table : magics[square - 1].moves + size;

        b = 0;
        size = 0;

        // Iterate over all subsets of the occupancy mask with the
        // Carry-Rippler trick and compute all attack bitboards for the current
        // square based on the occupancy.
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
        } while (b);

#ifndef USE_PEXT

        // Now loop until we find a magic that maps each occupancy to a correct
        // index in our magic table. We optimize the loop by using two binary
        // ANDs to reduce the number of significant bits in the magic
        // (good magics generally have high bit sparsity), and we reduce
        // further the range of tested values by removing magics which do not
        // generate enough significant bits for a full occupancy mask.
        for (int i = 0; i < size;)
        {
            for (m->magic = 0; popcount((m->magic * m->mask) >> 56) < 6;)
                m->magic = qrandom(&seed) & qrandom(&seed) & qrandom(&seed);

            // Check if the generated magic correctly maps each occupancy to
            // its corresponding bitboard attack. Note that we build the table
            // for the square as we test for each occupancy as a speedup, and
            // that we allow two different occupancies to map to the same index
            // if their corresponding bitboard attack is identical.
            for (++currentEpoch, i = 0; i < size; ++i)
            {
                unsigned int index = magic_index(m, occupancy[i]);

                // Check if we already wrote an attack bitboard at this index
                // during this iteration of the loop. If not, we can set
                // the attack bitboard corresponding to the occupancy at this
                // index; otherwise we check if the attack bitboard already
                // written is identical to the current one, and if it's not
                // the case, the mapping failed, and we must try another value.
                if (epoch[index] < currentEpoch)
                {
                    epoch[index] = currentEpoch;
                    m->moves[index] = reference[i];
                }
                else if (m->moves[index] != reference[i])
                    break;
            }
        }
#endif
    }
}

// Initializes all bitboard tables at program startup.
void bitboard_init(void)
{
    static const direction_t kingDirections[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
    static const direction_t knightDirections[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    static const direction_t bishopDirections[4] = {-9, -7, 7, 9};
    static const direction_t rookDirections[4] = {-8, -1, 1, 8};

    // Initialize the square distance table.
    for (square_t sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (square_t sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            const int fileDistance = abs(sq_file(sq1) - sq_file(sq2));
            const int rankDistance = abs(sq_rank(sq1) - sq_rank(sq2));

            SquareDistance[sq1][sq2] = imax(fileDistance, rankDistance);
        }

    // Initialize the Pawn pseudo-moves table.
    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        const bitboard_t b = square_bb(square);

        PawnMoves[WHITE][square] = (shift_up_left(b) | shift_up_right(b));
        PawnMoves[BLACK][square] = (shift_down_left(b) | shift_down_right(b));
    }

    // Initialize the King and Knight pseudo-moves tables.
    for (square_t square = SQ_A1; square <= SQ_H8; ++square)
    {
        for (int i = 0; i < 8; ++i)
        {
            const square_t to = square + kingDirections[i];

            if (is_valid_sq(to) && SquareDistance[square][to] <= 2)
                PseudoMoves[KING][square] |= square_bb(to);
        }

        for (int i = 0; i < 8; ++i)
        {
            const square_t to = square + knightDirections[i];

            if (is_valid_sq(to) && SquareDistance[square][to] <= 2)
                PseudoMoves[KNIGHT][square] |= square_bb(to);
        }
    }

    // Initialize the Bishop and Rook magic tables.
    magic_init(HiddenBishopTable, BishopMagics, bishopDirections);
    magic_init(HiddenRookTable, RookMagics, rookDirections);

    // Initialize the Bishop, Rook and Queen pseudo-moves tables, as well
    // as the line bitboards table.
    for (square_t sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
    {
        PseudoMoves[QUEEN][sq1] = PseudoMoves[BISHOP][sq1] = bishop_moves_bb(sq1, 0);
        PseudoMoves[QUEEN][sq1] |= PseudoMoves[ROOK][sq1] = rook_moves_bb(sq1, 0);

        for (square_t sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            if (PseudoMoves[BISHOP][sq1] & square_bb(sq2))
                LineBB[sq1][sq2] = (bishop_moves_bb(sq1, 0) & bishop_moves_bb(sq2, 0))
                                   | square_bb(sq1) | square_bb(sq2);

            if (PseudoMoves[ROOK][sq1] & square_bb(sq2))
                LineBB[sq1][sq2] = (rook_moves_bb(sq1, 0) & rook_moves_bb(sq2, 0)) | square_bb(sq1)
                                   | square_bb(sq2);
        }
    }
}
