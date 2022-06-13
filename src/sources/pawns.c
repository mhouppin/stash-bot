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

#include "pawns.h"
#include "evaluate.h"
#include "worker.h"

// clang-format off

const scorepair_t BackwardPenalty  = SPAIR( -5, -5);
const scorepair_t StragglerPenalty = SPAIR(-15,-22);
const scorepair_t DoubledPenalty   = SPAIR(-18,-32);
const scorepair_t IsolatedPenalty  = SPAIR( -9,-10);

const scorepair_t PassedBonus[RANK_NB] = {
    0,
    SPAIR(-12,-29),
    SPAIR( -6,-13),
    SPAIR( -4, 44),
    SPAIR( 21, 96),
    SPAIR( 47,172),
    SPAIR( 55,296),
    0
};

const scorepair_t PhalanxBonus[RANK_NB] = {
    0,
    SPAIR(  3,  7),
    SPAIR( 12,  6),
    SPAIR( 22, 27),
    SPAIR( 47, 57),
    SPAIR(161,216),
    SPAIR(182,219),
    0
};

const scorepair_t DefenderBonus[RANK_NB] = {
    0,
    SPAIR( 13, 20),
    SPAIR( 10, 20),
    SPAIR( 17, 26),
    SPAIR( 41, 68),
    SPAIR(155, 95),
    0,
    0
};

// clang-format on

scorepair_t evaluate_passed(
    pawn_entry_t *entry, color_t us, bitboard_t ourPawns, bitboard_t theirPawns)
{
    scorepair_t ret = 0;

    while (ourPawns)
    {
        square_t sq = bb_pop_first_sq(&ourPawns);
        bitboard_t queening = forward_file_bb(us, sq);

        TRACE_ADD(IDX_PIECE + PAWN - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + relative_sq(sq, us) - SQ_A2, us, 1);

        if ((queening & theirPawns) == 0
            && (queening & entry->attacks[not_color(us)] & ~entry->attacks[us]) == 0
            && (queening & entry->attacks2[not_color(us)] & ~entry->attacks2[us]) == 0)
        {
            ret += PassedBonus[relative_sq_rank(sq, us)];
            entry->passed[us] |= square_bb(sq);
            TRACE_ADD(IDX_PASSER + relative_sq_rank(sq, us) - RANK_2, us, 1);
        }
    }

    return (ret);
}

scorepair_t evaluate_backward(
    pawn_entry_t *entry, color_t us, bitboard_t ourPawns, bitboard_t theirPawns)
{
    bitboard_t stopSquares = (us == WHITE) ? shift_up(ourPawns) : shift_down(ourPawns);
    bitboard_t ourAttackSpan = 0;
    bitboard_t bb = ourPawns;

    while (bb) ourAttackSpan |= pawn_attack_span_bb(us, bb_pop_first_sq(&bb));

    // Save the pawn attack span to the entry.

    entry->attackSpan[us] = ourAttackSpan;

    bitboard_t theirAttacks =
        (us == WHITE) ? bpawns_attacks_bb(theirPawns) : wpawns_attacks_bb(theirPawns);
    bitboard_t backward = stopSquares & theirAttacks & ~ourAttackSpan;

    backward = (us == WHITE) ? shift_down(backward) : shift_up(backward);

    scorepair_t ret = 0;

    if (!backward) return (ret);

    ret += BackwardPenalty * popcount(backward);
    TRACE_ADD(IDX_BACKWARD, us, popcount(backward));

    backward &= (us == WHITE) ? (RANK_2_BITS | RANK_3_BITS) : (RANK_6_BITS | RANK_7_BITS);

    if (!backward) return (ret);

    bitboard_t theirFiles = 0;

    while (theirPawns) theirFiles |= forward_file_bb(not_color(us), bb_pop_first_sq(&theirPawns));

    backward &= ~theirFiles;

    if (!backward) return (ret);

    ret += StragglerPenalty * popcount(backward);
    TRACE_ADD(IDX_STRAGGLER, us, popcount(backward));

    return (ret);
}

scorepair_t evaluate_connected(bitboard_t bb, color_t us)
{
    scorepair_t ret = 0;

    bitboard_t phalanx = bb & shift_left(bb);

    while (phalanx)
    {
        square_t sq = bb_pop_first_sq(&phalanx);

        ret += PhalanxBonus[relative_sq_rank(sq, us)];
        TRACE_ADD(IDX_PHALANX + relative_sq_rank(sq, us) - RANK_2, us, 1);
    }

    bitboard_t defenders = bb & (us == WHITE ? bpawns_attacks_bb(bb) : wpawns_attacks_bb(bb));

    while (defenders)
    {
        square_t sq = bb_pop_first_sq(&defenders);

        ret += DefenderBonus[relative_sq_rank(sq, us)];
        TRACE_ADD(IDX_DEFENDER + relative_sq_rank(sq, us) - RANK_2, us, 1);
    }

    return (ret);
}

scorepair_t evaluate_doubled_isolated(bitboard_t bb, color_t us __attribute__((unused)))
{
    scorepair_t ret = 0;

    for (square_t s = SQ_A2; s <= SQ_H2; ++s)
    {
        bitboard_t b = bb & sq_file_bb(s);

        if (b)
        {
            if (more_than_one(b))
            {
                ret += DoubledPenalty;
                TRACE_ADD(IDX_DOUBLED, us, 1);
            }
            if (!(adjacent_files_bb(s) & bb))
            {
                ret += IsolatedPenalty;
                TRACE_ADD(IDX_ISOLATED, us, 1);
            }
        }
    }

    return (ret);
}

pawn_entry_t *pawn_probe(const board_t *board)
{
#ifndef TUNE
    pawn_entry_t *entry = get_worker(board)->pawnTable + (board->stack->pawnKey % PawnTableSize);

    if (entry->key == board->stack->pawnKey) return (entry);

#else
    static pawn_entry_t e;
    pawn_entry_t *entry = &e;
#endif

    entry->key = board->stack->pawnKey;
    entry->value = 0;
    entry->attackSpan[WHITE] = entry->attackSpan[BLACK] = 0;
    entry->passed[WHITE] = entry->passed[BLACK] = 0;

    const bitboard_t wpawns = piece_bb(board, WHITE, PAWN);
    const bitboard_t bpawns = piece_bb(board, BLACK, PAWN);

    entry->attacks[WHITE] = wpawns_attacks_bb(wpawns);
    entry->attacks[BLACK] = bpawns_attacks_bb(bpawns);
    entry->attacks2[WHITE] = wpawns_2attacks_bb(wpawns);
    entry->attacks2[BLACK] = bpawns_2attacks_bb(bpawns);

    entry->value += evaluate_backward(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_backward(entry, BLACK, bpawns, wpawns);
    entry->value += evaluate_connected(wpawns, WHITE);
    entry->value -= evaluate_connected(bpawns, BLACK);
    entry->value += evaluate_passed(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_passed(entry, BLACK, bpawns, wpawns);
    entry->value += evaluate_doubled_isolated(wpawns, WHITE);
    entry->value -= evaluate_doubled_isolated(bpawns, BLACK);

    return (entry);
}
