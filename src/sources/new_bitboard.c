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

#include "new_bitboard.h"

#ifdef USE_PEXT
// Do not include the header if USE_PEXT isn't specified, because we don't need
// it in this case, and it might increase compilation time drastically.
#include <immintrin.h>
#endif

#include <stdio.h>

#include "new_attacks.h"
#include "new_chess_types.h"
#include "new_core.h"
#include "new_random.h"

Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard RawAttacks[PIECETYPE_NB][SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
Magic BishopMagics[SQUARE_NB];
Magic RookMagics[SQUARE_NB];
u8 SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard HiddenRookAttackTable[0x19000];
Bitboard HiddenBishopAttackTable[0x1480];

u32 magic_index(const Magic *magic, Bitboard occupancy) {
#ifdef USE_PEXT
    return _pext_u64(occupancy, magic->mask);
#else
    return (u32)(((occupancy & magic->mask) * magic->magic) >> magic->shift);
#endif
}

// Returns a bitboard of all the reachable squares by a bishop/rook from the given square and board
// occupancy
Bitboard sliding_attacks_bb(const Direction *directions, Square square, Bitboard occupancy) {
    Bitboard attacks = 0;

    for (u8 i = 0; i < 4; ++i) {
        for (Square sqi = square + directions[i];
             square_is_valid(sqi) && square_distance(sqi, sqi - directions[i]) == 1;
             sqi += directions[i]) {
            bb_set_square(&attacks, sqi);
            if (bb_square_is_set(occupancy, sqi)) {
                break;
            }
        }
    }

    return attacks;
}

// Initializes magic bitboard tables for bishop rook and queen attacks
void magic_init(Bitboard *attack_table, Magic *magic_table, const Direction directions[4]) {
    Bitboard reference[1 << 12], edges, bb;
    u32 size = 0;

#ifndef USE_PEXT
    Bitboard occupancy[1 << 12];

    // The epoch is used to determine which iteration of the occupancy test we are in, to avoid
    // having to zero the attack array between each failed iteration.
    u32 epoch_table[1 << 12] = {0}, current_epoch = 0;
    u64 seed = 64;
#endif

    for (Square square = SQ_A1; square <= SQ_H8; ++square) {
        Magic *magic_entry = &magic_table[square];

        // The edges of the board are not counted in the occupancy bits since we can reach them
        // whether there's a piece on them or not because of capture moves, but we must still ensure
        // they're accounted for if the piece is already on the edge for Rook moves.
        edges = ((RANK_1_BB | RANK_8_BB) & ~square_rank_bb(square))
            | ((FILE_A_BB | FILE_H_BB) & ~square_file_bb(square));

        magic_entry->mask = sliding_attacks_bb(directions, square, 0) & ~edges;

        // We will need popcount(mask) bits of information for indexing the occupancy, and (1 <<
        // popcount(mask)) entries in the table for storing the corresponding attack bitboards.
        magic_entry->shift = 64 - bb_popcount(magic_entry->mask);
        magic_entry->moves =
            (square == SQ_A1) ? attack_table : magic_table[square - 1].moves + size;

        bb = 0;
        size = 0;

        // Iterate over all subsets of the occupancy mask with the Carry-Rippler trick and compute
        // all attack bitboards for the current square based on the occupancy.
        do {
#ifndef USE_PEXT
            occupancy[size] = bb;
#endif

            reference[size] = sliding_attacks_bb(directions, square, bb);

#ifdef USE_PEXT
            magic_entry->moves[__builtin_ia32_pext_di(bb, magic_entry->mask)] = reference[size];
#endif

            ++size;
            bb = (bb - magic_entry->mask) & magic_entry->mask;
        } while (bb);

#ifndef USE_PEXT

        // Now loop until we find a magic that maps each occupancy to a correct index in our magic
        // table. We optimize the loop by using two binary ANDs to reduce the number of significant
        // bits in the magic (good magics generally have high bit sparsity), and we reduce further
        // the range of tested values by removing magics which do not generate enough significant
        // bits for a full occupancy mask.
        for (u32 i = 0; i < size;) {
            for (magic_entry->magic = 0;
                 u64_count_ones((magic_entry->magic * magic_entry->mask) >> 56) < 6;) {
                magic_entry->magic = u64_random(&seed) & u64_random(&seed) & u64_random(&seed);
            }

            // Check if the generated magic correctly maps each occupancy to its corresponding
            // bitboard attack. Note that we build the table for the square as we test for each
            // occupancy as a speedup, and that we allow two different occupancies to map to the
            // same index if their corresponding bitboard attack is identical.
            for (++current_epoch, i = 0; i < size; ++i) {
                u32 index = magic_index(magic_entry, occupancy[i]);

                // Check if we already wrote an attack bitboard at this index during this iteration
                // of the loop. If not, we can set the attack bitboard corresponding to the
                // occupancy at this index; otherwise we check if the attack bitboard already
                // written is identical to the current one, and if it's not the case, the mapping
                // failed, and we must try another value.
                if (epoch_table[index] < current_epoch) {
                    epoch_table[index] = current_epoch;
                    magic_entry->moves[index] = reference[i];
                } else if (magic_entry->moves[index] != reference[i]) {
                    break;
                }
            }
        }
#endif
    }
}

// Helper for Knight and King attack tables.
void initialize_raw_attacks(Piecetype piecetype, const Direction directions[8]) {
    for (Square square = SQ_A1; square <= SQ_H8; ++square)
        for (u8 i = 0; i < 8; ++i) {
            const Square to = square + directions[i];

            if (square_is_valid(to) && square_distance(square, to) <= 2)
                bb_set_square(&RawAttacks[piecetype][square], to);
        }
}

// Helper for Bishop and Rook raw attack tables.
void initialize_raw_slider_attacks(Piecetype piecetype) {
    for (Square square = SQ_A1; square <= SQ_H8; ++square) {
        RawAttacks[piecetype][square] = attacks_bb(piecetype, square, 0);

        for (Square to = SQ_A1; to <= SQ_H8; ++to) {
            if (bb_square_is_set(RawAttacks[piecetype][square], to)) {
                LineBB[square][to] =
                    attacks_bb(piecetype, square, 0) & attacks_bb(piecetype, to, 0);
                LineBB[square][to] |= square_bb(square) | square_bb(to);
            }
        }
    }
}

void bitboard_init(void) {
    static const Direction king_directions[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
    static const Direction knight_directions[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    static const Direction bishop_directions[4] = {-9, -7, 7, 9};
    static const Direction rook_directions[4] = {-8, -1, 1, 8};

    for (Square square1 = SQ_A1; square1 <= SQ_H8; ++square1)
        for (Square square2 = SQ_A1; square2 <= SQ_H8; ++square2) {
            u8 file_dist = square_file_distance(square1, square2);
            u8 rank_dist = square_rank_distance(square1, square2);
            SquareDistance[square1][square2] = u8_max(file_dist, rank_dist);
        }

    for (Square square = SQ_A1; square <= SQ_H8; ++square) {
        Bitboard bb = square_bb(square);

        PawnAttacks[WHITE][square] = bb_shift_up_left(bb) | bb_shift_up_right(bb);
        PawnAttacks[BLACK][square] = bb_shift_down_left(bb) | bb_shift_down_right(bb);
    }

    initialize_raw_attacks(KNIGHT, knight_directions);
    initialize_raw_attacks(KING, king_directions);
    magic_init(HiddenBishopAttackTable, BishopMagics, bishop_directions);
    magic_init(HiddenRookAttackTable, RookMagics, rook_directions);

    initialize_raw_slider_attacks(BISHOP);
    initialize_raw_slider_attacks(ROOK);

    for (Square square = SQ_A1; square <= SQ_H8; ++square)
        RawAttacks[QUEEN][square] = RawAttacks[BISHOP][square] | RawAttacks[ROOK][square];
}
