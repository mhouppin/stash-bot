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

#include "kpk_bitbase.h"

#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "bitboard.h"
#include "wmalloc.h"

static u8 KpkBitbase[KPK_SIZE / 8];

static u32 kpk_index(Square weak_ksq, Square strong_ksq, Square psq, Color stm) {
    assert(square_is_valid(weak_ksq));
    assert(square_is_valid(strong_ksq));
    assert(square_is_valid(psq));
    assert(square_file(psq) < FILE_E);
    assert(square_rank(psq) != RANK_1 && square_rank(psq) != RANK_8);
    assert(color_is_valid(stm));
    return (u32)weak_ksq | ((u32)strong_ksq << 6) | ((u32)stm << 12) | ((u32)square_file(psq) << 13)
        | ((u32)(RANK_7 - square_rank(psq)) << 15);
}

static void kpk_init_entry(KpkPosition *pos, u32 index) {
    assert(index < KPK_SIZE);
    const Square weak_ksq = (Square)(index & 0b111111u);
    const Square strong_ksq = (Square)((index >> 6) & 0b111111u);
    const Color stm = (Color)((index >> 12) & 0b1u);
    const Square psq = create_square((File)((index >> 13) & 0b11u), (Rank)(RANK_7 - (index >> 15)));

    pos->stm = stm;
    pos->ksq[WHITE] = strong_ksq;
    pos->ksq[BLACK] = weak_ksq;
    pos->psq = psq;
    pos->result = KPK_UNKNOWN;

    // Test for overlapping pieces.
    if (square_distance(strong_ksq, weak_ksq) <= 1 || weak_ksq == psq || strong_ksq == psq) {
        pos->result = KPK_INVALID;
    } else if (stm == WHITE) {
        // Test if the weak King is in check while the strong side has the move.
        if (bb_square_is_set(pawn_attacks_bb(psq, WHITE), weak_ksq)) {
            pos->result = KPK_INVALID;
        }
        // Test if we can promote the Pawn without getting captured.
        else if (square_rank(psq) == RANK_7 && strong_ksq != psq + NORTH
                 && (square_distance(weak_ksq, psq + NORTH) > 1
                     || square_distance(strong_ksq, psq + NORTH) == 1)) {
            pos->result = KPK_WIN;
        }
    } else {
        // Test for a stalemate.
        if (!(king_attacks_bb(weak_ksq)
              & ~(king_attacks_bb(strong_ksq) | pawn_attacks_bb(psq, WHITE)))) {
            pos->result = KPK_DRAW;
        }
        // Test if the weak King can capture the Pawn.
        else if (bb_square_is_set(king_attacks_bb(weak_ksq) & ~king_attacks_bb(strong_ksq), psq)) {
            pos->result = KPK_DRAW;
        }
    }
}

void kpk_classify(KpkPosition *pos, KpkPosition *kpk_table) {
    const u8 good_result = (pos->stm == WHITE) ? KPK_WIN : KPK_DRAW;
    const u8 bad_result = good_result ^ KPK_DRAW ^ KPK_WIN;
    const Square strong_ksq = pos->ksq[WHITE];
    const Square weak_ksq = pos->ksq[BLACK];
    const Color stm = pos->stm;
    const Square psq = pos->psq;

    // We will pack all moves' results in the result variable with bitwise ORs. We use the fact that
    // invalid entries with overlapping pieces are stored as KPK_INVALID (aka 0) to avoid checking
    // for move legality (expect for double Pawn pushes, where we need to check if the square above
    // the pawn is empty).
    u8 result = KPK_INVALID;
    Bitboard bb = king_attacks_bb(pos->ksq[stm]);

    // Get all entries' results for King moves.
    while (bb) {
        const Square next_strong_ksq = (stm == WHITE) ? bb_pop_first_square(&bb) : strong_ksq;
        const Square next_weak_ksq = (stm == WHITE) ? weak_ksq : bb_pop_first_square(&bb);

        result |= kpk_table[kpk_index(next_weak_ksq, next_strong_ksq, psq, color_flip(stm))].result;
    }

    // If the strong side has the move, also get all entries' results for Pawn moves.
    if (stm == WHITE) {
        if (square_rank(psq) < RANK_7) {
            result |= kpk_table[kpk_index(weak_ksq, strong_ksq, psq + NORTH, BLACK)].result;
        }

        if (square_rank(psq) == RANK_2 && psq + NORTH != strong_ksq && psq + NORTH != weak_ksq) {
            result |= kpk_table[kpk_index(weak_ksq, strong_ksq, psq + NORTH + NORTH, BLACK)].result;
        }
    }

    // If we can get the "good result" with at least one move, we have a good result. Otherwise, we
    // have a "bad result", except if at least one of the moves has an unknown result.
    pos->result = (result & good_result) ? good_result
        : (result & KPK_UNKNOWN)         ? KPK_UNKNOWN
                                         : bad_result;
}

void kpk_bitbase_init(void) {
    KpkPosition *kpk_table = wrap_malloc(sizeof(KpkPosition) * KPK_SIZE);
    u32 index;
    bool table_modified;

    // Fill the bitbase with zeros, and then perform an early recognition of trivial wins/draws and
    // illegal positions.
    memset(KpkBitbase, 0, sizeof(KpkBitbase));

    for (index = 0; index < KPK_SIZE; ++index) {
        kpk_init_entry(&kpk_table[index], index);
    }

    // Classify all undecided positions by retrograde analysis.
    do {
        table_modified = false;

        for (index = 0; index < KPK_SIZE; ++index) {
            if (kpk_table[index].result == KPK_UNKNOWN) {
                kpk_classify(&kpk_table[index], kpk_table);
                table_modified |= kpk_table[index].result != KPK_UNKNOWN;
            }
        }
    } while (table_modified);

    // Index the wins in the bitbase as set bits.
    for (index = 0; index < KPK_SIZE; ++index) {
        if (kpk_table[index].result == KPK_WIN) {
            KpkBitbase[index >> 3] |= 1u << (index & 7);
        }
    }

    free(kpk_table);
}

bool kpk_bitbase_is_winning(Square weak_ksq, Square strong_ksq, Square psq, Color stm) {
    u32 index = kpk_index(weak_ksq, strong_ksq, psq, stm);
    return (KpkBitbase[index >> 3] & (1u << (index & 7))) != 0;
}
