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

#include "pawns.h"
#include "evaluate.h"
#include "worker.h"

// clang-format off

// Miscellanous bonus for Pawn structures
const scorepair_t BackwardPenalty  = SPAIR( -5, -8);
const scorepair_t StragglerPenalty = SPAIR(-22,-23);
const scorepair_t DoubledPenalty   = SPAIR( -3,-39);
const scorepair_t IsolatedPenalty  = SPAIR(-10,-12);

// Rank-based bonus for passed Pawns
const scorepair_t PassedBonus[8] = {
    0,
    SPAIR(-17,  7),
    SPAIR(-17, 16),
    SPAIR(-12, 50),
    SPAIR( 18,115),
    SPAIR( 43,222),
    SPAIR( 69,343),
    0
};

// Passed Pawn eval terms
const scorepair_t PP_OurKingProximity[24] = {
    SPAIR(   4,  68), SPAIR(   5,  20), SPAIR( -18, -80),
    SPAIR(   0,   0), SPAIR(   0,   0), SPAIR(   0,   0),
    SPAIR(   6,  87), SPAIR(   6,  25), SPAIR(   6, -23),
    SPAIR( -20, -60), SPAIR(   0,   0), SPAIR(   0,   0),
    SPAIR(   2,  65), SPAIR(  -9,  31), SPAIR( -12,  -5),
    SPAIR(  -3, -37), SPAIR(  23, -37), SPAIR(   0,   0),
    SPAIR( -10,  48), SPAIR( -18,  28), SPAIR(  -7,  -8),
    SPAIR(  -3, -12), SPAIR(  14, -18), SPAIR(  29, -23)
};

const scorepair_t PP_TheirKingProximity[24] = {
    SPAIR( -28,-164), SPAIR(   7,   9), SPAIR(  11, 163),
    SPAIR(   0,   0), SPAIR(   0,   0), SPAIR(   0,   0),
    SPAIR( -21,-145), SPAIR(  31, -51), SPAIR(  -8,  78),
    SPAIR(  -3, 147), SPAIR(   0,   0), SPAIR(   0,   0),
    SPAIR(  -6, -78), SPAIR(  13, -40), SPAIR(  14, -12),
    SPAIR(  -3,  60), SPAIR( -17,  88), SPAIR(   0,   0),
    SPAIR(  -5, -44), SPAIR(  -5, -12), SPAIR(   5,  -9),
    SPAIR(  13,  -8), SPAIR(  -4,  43), SPAIR(  -2,  43)
};

// Rank-based bonus for phalanx structures
const scorepair_t PhalanxBonus[8] = {
    0,
    SPAIR( 17, -2),
    SPAIR( 23, 10),
    SPAIR( 35, 28),
    SPAIR( 61, 64),
    SPAIR(167,242),
    SPAIR(183,236),
    0
};

// Rank-based bonus for defenders
const scorepair_t DefenderBonus[8] = {
    0,
    SPAIR( 26, 22),
    SPAIR( 19, 24),
    SPAIR( 18, 36),
    SPAIR( 31, 91),
    SPAIR(166,134),
    0,
    0
};

// clang-format on

scorepair_t evaluate_passed(
    KingPawnEntry *entry, color_t us, bitboard_t ourPawns, bitboard_t theirPawns)
{
    scorepair_t ret = 0;

    while (ourPawns)
    {
        square_t sq = bb_pop_first_sq(&ourPawns);
        bitboard_t queening = forward_file_bb(us, sq);

        TRACE_ADD(IDX_PIECE + PAWN - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + relative_sq(sq, us) - SQ_A2, us, 1);

        // Check if our Pawn has a free path to its queening square, or if every
        // square attacked by an enemy Pawn is matched by a friendly Pawn
        // attacking it too.
        if ((queening & theirPawns) == 0
            && (queening & entry->attacks[not_color(us)] & ~entry->attacks[us]) == 0
            && (queening & entry->attacks2[not_color(us)] & ~entry->attacks2[us]) == 0)
        {
            ret += PassedBonus[relative_sq_rank(sq, us)];
            entry->passed[us] |= square_bb(sq);
            TRACE_ADD(IDX_PASSER + relative_sq_rank(sq, us) - RANK_2, us, 1);
        }
    }

    return ret;
}

scorepair_t evaluate_passed_pos(const KingPawnEntry *entry, const Board *board, color_t us)
{
    scorepair_t ret = 0;
    square_t ourKing = get_king_square(board, us);
    square_t theirKing = get_king_square(board, not_color(us));
    bitboard_t bb = entry->passed[us];

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        int queeningDistance = RANK_8 - relative_sq_rank(sq, us);

        // Give a bonus/penalty based on how close our King and their King are
        // from the Pawn.
        if (queeningDistance <= 4)
        {
            int ourDistance = SquareDistance[ourKing][sq];
            int theirDistance = SquareDistance[theirKing][sq];

            int ourIndex = (queeningDistance - 1) * 6 + imin(queeningDistance + 2, ourDistance) - 1;
            int theirIndex =
                (queeningDistance - 1) * 6 + imin(queeningDistance + 2, theirDistance) - 1;

            ret += PP_OurKingProximity[ourIndex];
            ret += PP_TheirKingProximity[theirIndex];

            TRACE_ADD(IDX_PP_OUR_KING_PROX + ourIndex, us, 1);
            TRACE_ADD(IDX_PP_THEIR_KING_PROX + theirIndex, us, 1);
        }
    }

    return ret;
}

scorepair_t evaluate_backward(
    KingPawnEntry *entry, color_t us, bitboard_t ourPawns, bitboard_t theirPawns)
{
    bitboard_t stopSquares = (us == WHITE) ? shift_up(ourPawns) : shift_down(ourPawns);
    bitboard_t ourAttackSpan = 0;
    bitboard_t bb = ourPawns;

    while (bb) ourAttackSpan |= pawn_attack_span_bb(us, bb_pop_first_sq(&bb));

    // Save the pawn attack span to the entry for later use.
    entry->attackSpan[us] = ourAttackSpan;

    bitboard_t theirAttacks =
        (us == WHITE) ? bpawns_attacks_bb(theirPawns) : wpawns_attacks_bb(theirPawns);
    bitboard_t backward = stopSquares & theirAttacks & ~ourAttackSpan;

    backward = (us == WHITE) ? shift_down(backward) : shift_up(backward);

    scorepair_t ret = 0;

    if (!backward) return ret;

    // Penalty for Pawns which cannot advance due to their stop square being
    // attacked by enemy Pawns, with none of our Pawns able to defend it.
    ret += BackwardPenalty * popcount(backward);
    TRACE_ADD(IDX_BACKWARD, us, popcount(backward));

    backward &= (us == WHITE) ? (RANK_2_BB | RANK_3_BB) : (RANK_6_BB | RANK_7_BB);

    if (!backward) return ret;

    bitboard_t theirFiles = 0;

    while (theirPawns) theirFiles |= forward_file_bb(not_color(us), bb_pop_first_sq(&theirPawns));

    backward &= ~theirFiles;

    if (!backward) return ret;

    // Additional penalty for exposed backward Pawns on the second or third rank.
    ret += StragglerPenalty * popcount(backward);
    TRACE_ADD(IDX_STRAGGLER, us, popcount(backward));

    return ret;
}

scorepair_t evaluate_connected(bitboard_t bb, color_t us)
{
    scorepair_t ret = 0;

    bitboard_t phalanx = bb & shift_left(bb);

    // Bonus for Pawns sitting next to each other.
    while (phalanx)
    {
        square_t sq = bb_pop_first_sq(&phalanx);

        ret += PhalanxBonus[relative_sq_rank(sq, us)];
        TRACE_ADD(IDX_PHALANX + relative_sq_rank(sq, us) - RANK_2, us, 1);
    }

    bitboard_t defenders = bb & (us == WHITE ? bpawns_attacks_bb(bb) : wpawns_attacks_bb(bb));

    // Bonus for Pawns supporting other Pawns.
    while (defenders)
    {
        square_t sq = bb_pop_first_sq(&defenders);

        ret += DefenderBonus[relative_sq_rank(sq, us)];
        TRACE_ADD(IDX_DEFENDER + relative_sq_rank(sq, us) - RANK_2, us, 1);
    }

    return ret;
}

scorepair_t evaluate_doubled_isolated(bitboard_t bb, color_t us __attribute__((unused)))
{
    scorepair_t ret = 0;

    for (square_t s = SQ_A2; s <= SQ_H2; ++s)
    {
        bitboard_t b = bb & sq_file_bb(s);

        if (b)
        {
            // Penalty for Pawns on the same file.
            if (more_than_one(b))
            {
                ret += DoubledPenalty;
                TRACE_ADD(IDX_DOUBLED, us, 1);
            }

            // Penalty for Pawns with no friendly Pawn on adjacent files.
            if (!(adjacent_files_bb(s) & bb))
            {
                ret += IsolatedPenalty;
                TRACE_ADD(IDX_ISOLATED, us, 1);
            }
        }
    }

    return ret;
}

KingPawnEntry *kp_probe(const Board *board)
{
#ifndef TUNE
    // Check if this pawn structure has already been evaluated.
    KingPawnEntry *entry =
        get_worker(board)->kingPawnTable + (board->stack->kingPawnKey % KingPawnTableSize);

    if (entry->key == board->stack->kingPawnKey) return entry;

#else
    static KingPawnEntry e;
    KingPawnEntry *entry = &e;
#endif

    // Reset the entry contents.
    entry->key = board->stack->kingPawnKey;
    entry->value = 0;
    entry->attackSpan[WHITE] = entry->attackSpan[BLACK] = 0;
    entry->passed[WHITE] = entry->passed[BLACK] = 0;

    const bitboard_t wpawns = piece_bb(board, WHITE, PAWN);
    const bitboard_t bpawns = piece_bb(board, BLACK, PAWN);

    // Compute and save the Pawn direct attacks for later use.
    entry->attacks[WHITE] = wpawns_attacks_bb(wpawns);
    entry->attacks[BLACK] = bpawns_attacks_bb(bpawns);
    entry->attacks2[WHITE] = wpawns_2attacks_bb(wpawns);
    entry->attacks2[BLACK] = bpawns_2attacks_bb(bpawns);

    // Evaluate the Pawn structure.
    entry->value += evaluate_backward(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_backward(entry, BLACK, bpawns, wpawns);

    entry->value += evaluate_connected(wpawns, WHITE);
    entry->value -= evaluate_connected(bpawns, BLACK);

    entry->value += evaluate_passed(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_passed(entry, BLACK, bpawns, wpawns);

    entry->value += evaluate_passed_pos(entry, board, WHITE);
    entry->value -= evaluate_passed_pos(entry, board, BLACK);

    entry->value += evaluate_doubled_isolated(wpawns, WHITE);
    entry->value -= evaluate_doubled_isolated(bpawns, BLACK);

    // Return the entry pointer since the evaluation will make use of some of
    // the fields (like the Pawn attack span).
    return entry;
}
