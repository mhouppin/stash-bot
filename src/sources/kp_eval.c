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

#include "kp_eval.h"

#include "attacks.h"
#include "evaluate.h"
#include "worker.h"

// clang-format off

// Miscellanous bonus for Pawn structures
const Scorepair BackwardPenalty  = SPAIR( -4, -10);
const Scorepair StragglerPenalty = SPAIR(-15, -20);
const Scorepair DoubledPenalty   = SPAIR(-11, -52);
const Scorepair IsolatedPenalty  = SPAIR( -7,  -8);

// Rank-based bonus for passed Pawns
const Scorepair PassedBonus[8] = {
    0,
    SPAIR( -8,   4),
    SPAIR(-12,  13),
    SPAIR(-25,  53),
    SPAIR( 10, 114),
    SPAIR( 41, 211),
    SPAIR( 69, 353),
    0
};

// Passed Pawn eval terms
const Scorepair PassedOurKingDistance[24] = {
    SPAIR(   8,   95), SPAIR(  10,   16), SPAIR( -29,  -94),
    SPAIR(   0,    0), SPAIR(   0,    0), SPAIR(   0,    0),
    SPAIR(  12,   82), SPAIR(  12,   28), SPAIR(   9,  -31),
    SPAIR( -38,  -63), SPAIR(   0,    0), SPAIR(   0,    0),
    SPAIR(   3,   78), SPAIR( -25,   42), SPAIR( -21,   -8),
    SPAIR(  -3,  -45), SPAIR(  37,  -51), SPAIR(   0,    0),
    SPAIR( -32,   58), SPAIR( -31,   33), SPAIR( -10,  -10),
    SPAIR(   2,  -14), SPAIR(  25,  -22), SPAIR(  42,  -28)
};

const Scorepair PassedTheirKingDistance[24] = {
    SPAIR( -15, -173), SPAIR(   5,   12), SPAIR(  -3,  179),
    SPAIR(   0,    0), SPAIR(   0,    0), SPAIR(   0,    0),
    SPAIR( -26, -147), SPAIR(  19,  -36), SPAIR(   5,   64),
    SPAIR(   0,  137), SPAIR(   0,    0), SPAIR(   0,    0),
    SPAIR( -11,  -95), SPAIR(  23,  -32), SPAIR(  14,   -1),
    SPAIR(  -5,   56), SPAIR( -31,   88), SPAIR(   0,    0),
    SPAIR( -15,  -46), SPAIR( -12,   -8), SPAIR(   6,   -5),
    SPAIR(  25,   -5), SPAIR( -10,   40), SPAIR(  -6,   38)
};

// Rank-based bonus for phalanx structures
const Scorepair PhalanxBonus[8] = {
    0,
    SPAIR(  5,  -3),
    SPAIR( 16,   9),
    SPAIR( 22,  31),
    SPAIR( 43,  64),
    SPAIR(171, 254),
    SPAIR(183, 244),
    0
};

// Rank-based bonus for defenders
const Scorepair DefenderBonus[8] = {
    0,
    SPAIR( 16,  22),
    SPAIR( 14,  23),
    SPAIR( 24,  36),
    SPAIR( 56,  99),
    SPAIR(174, 160),
    0,
    0
};

// clang-format on

static Bitboard compute_attack_span(Bitboard our_pawns, Color us) {
    Bitboard our_attack_span = 0;

    while (our_pawns) {
        our_attack_span |= pawn_attack_span_bb(bb_pop_first_square(&our_pawns), us);
    }

    return our_attack_span;
}

static Scorepair evaluate_backward(
    KingPawnEntry *restrict kpe,
    const PawnLocalData *restrict data,
    Color us,
    Bitboard our_pawns,
    Bitboard their_pawns
) {
    const Color them = color_flip(us);
    const Bitboard stop_squares = bb_shift_up_relative(our_pawns, us);
    Bitboard backward_pawns =
        bb_shift_down_relative(stop_squares & data->attacks[them] & ~kpe->attack_span[us], us);
    Scorepair ret = 0;

    if (!backward_pawns) {
        return ret;
    }

    // Penalize the pawns which cannot advance due to their stop square being attacked by enemy
    // pawns, with none of our pawns able to defend it.
    ret += BackwardPenalty * bb_popcount(backward_pawns);
    trace_add(IDX_BACKWARD, us, bb_popcount(backward_pawns));

    // For stragglers, we only consider the relative second and third ranks.
    backward_pawns &= (us == WHITE) ? (RANK_2_BB | RANK_3_BB) : (RANK_6_BB | RANK_7_BB);

    if (!backward_pawns) {
        return ret;
    }

    Bitboard closed_files = 0;

    while (their_pawns) {
        closed_files |= forward_file_bb(bb_pop_first_square(&their_pawns), them);
    }

    backward_pawns &= ~closed_files;

    if (!backward_pawns) {
        return ret;
    }

    // Add an extra penalty for exposed backward pawns on the second or third rank.
    ret += StragglerPenalty * bb_popcount(backward_pawns);
    trace_add(IDX_STRAGGLER, us, bb_popcount(backward_pawns));
    return ret;
}

static Scorepair evaluate_connected(Bitboard our_pawns, Color us) {
    Scorepair ret = 0;
    Bitboard phalanx = our_pawns & bb_shift_left(our_pawns);
    Bitboard defenders = our_pawns & pawns_attacks_bb(our_pawns, color_flip(us));

    // Give a bonus for pawns sitting next to each other.
    while (phalanx) {
        const Square sq = bb_pop_first_square(&phalanx);

        ret += PhalanxBonus[square_rank_relative(sq, us)];
        trace_add(IDX_PHALANX + square_rank_relative(sq, us) - RANK_2, us, 1);
    }

    // Give a bonus for pawns supporting other pawns.
    while (defenders) {
        const Square sq = bb_pop_first_square(&defenders);

        ret += DefenderBonus[square_rank_relative(sq, us)];
        trace_add(IDX_DEFENDER + square_rank_relative(sq, us) - RANK_2, us, 1);
    }

    return ret;
}

static Scorepair evaluate_passed(
    KingPawnEntry *restrict kpe,
    const PawnLocalData *restrict data,
    Color us,
    Bitboard our_pawns,
    Bitboard their_pawns
) {
    const Color them = color_flip(us);
    Scorepair ret = 0;

    while (our_pawns) {
        const Square sq = bb_pop_first_square(&our_pawns);
        const Bitboard queening_path = forward_file_bb(sq, us);

        trace_add(IDX_PIECE + PAWN - PAWN, us, 1);
        trace_add(IDX_PSQT + square_relative(sq, us) - SQ_A2, us, 1);

        // Check if our pawn has a free path to its queening square, or if every square attacked by
        // an enemy pawn is matched by a friendly pawn attacking it too.
        if (!(queening_path & their_pawns)
            && !(queening_path & data->attacks[them] & ~data->attacks[us])
            && !(queening_path & data->attacks2[them] & ~data->attacks2[us])) {
            ret += PassedBonus[square_rank_relative(sq, us)];
            bb_set_square(&kpe->passed[us], sq);
            trace_add(IDX_PASSER + square_rank_relative(sq, us) - RANK_2, us, 1);
        }
    }

    return ret;
}

// Converts a (queening_distance, king_distance) pair to the corresponding index in the
// PassedKingDistance tables.
static u8 distance_to_pkd_index(u8 queening_distance, u8 king_distance) {
    return (queening_distance - 1) * 6 + u8_min(queening_distance + 2, king_distance) - 1;
}

static Scorepair evaluate_passed_pos(const KingPawnEntry *kpe, const Board *board, Color us) {
    Scorepair ret = 0;
    const Square our_king = board_king_square(board, us);
    const Square their_king = board_king_square(board, color_flip(us));
    Bitboard bb = kpe->passed[us];

    while (bb) {
        const Square sq = bb_pop_first_square(&bb);
        const u8 queening_distance = RANK_8 - square_rank_relative(sq, us);

        // Give a bonus/penalty based on how close our King and their King are from the pawn.
        if (queening_distance <= 4) {
            const u8 our_distance = square_distance(our_king, sq);
            const u8 their_distance = square_distance(their_king, sq);
            const u8 our_index = distance_to_pkd_index(queening_distance, our_distance);
            const u8 their_index = distance_to_pkd_index(queening_distance, their_distance);

            ret += PassedOurKingDistance[our_index];
            ret += PassedTheirKingDistance[their_index];
            trace_add(IDX_PASSED_OUR_KING_DIST + our_index, us, 1);
            trace_add(IDX_PASSED_THEIR_KING_DIST + their_index, us, 1);
        }
    }

    return ret;
}

static Scorepair evaluate_doubled_isolated(Bitboard our_pawns, Color us __attribute__((unused))) {
    Scorepair ret = 0;

    for (File f = FILE_A; f <= FILE_H; ++f) {
        const Bitboard fbb = file_bb(f);
        Bitboard file_pawns = our_pawns & fbb;

        if (file_pawns) {
            // Penalize pawns being on the same file.
            if (bb_more_than_one(file_pawns)) {
                ret += DoubledPenalty;
                trace_add(IDX_DOUBLED, us, 1);
            }

            // Penalize pawns with no friendly pawn on adjacent files.
            if (!((bb_shift_left(fbb) | bb_shift_right(fbb)) & our_pawns)) {
                ret += IsolatedPenalty;
                trace_add(IDX_ISOLATED, us, 1);
            }
        }
    }

    return ret;
}

KingPawnEntry *king_pawn_probe(const Board *board) {
    // Required if we are calling evaluate() from the UCI thread or during tuning runs.
    static KingPawnEntry nocache;
    KingPawnEntry *kpe;

    if (!board->has_worker) {
        kpe = &nocache;
    } else {
        kpe = &board_get_worker(board)
                   ->king_pawn_table->entry[board->stack->king_pawn_key % KING_PAWN_ENTRY_NB];

        // Check if this pawn structure has already been evaluated.
        if (kpe->key == board->stack->king_pawn_key) {
            return kpe;
        }
    }

    PawnLocalData data;
    const Bitboard wpawns = board_piece_bb(board, WHITE, PAWN);
    const Bitboard bpawns = board_piece_bb(board, BLACK, PAWN);

    // Reset the entry contents.
    kpe->key = board->stack->king_pawn_key;
    kpe->value = 0;
    kpe->attack_span[WHITE] = compute_attack_span(wpawns, WHITE);
    kpe->attack_span[BLACK] = compute_attack_span(bpawns, BLACK);
    kpe->passed[WHITE] = kpe->passed[BLACK] = 0;

    // Compute and save the Pawn direct attacks for later use.
    data.attacks[WHITE] = white_pawns_attacks_bb(wpawns);
    data.attacks[BLACK] = black_pawns_attacks_bb(bpawns);
    data.attacks2[WHITE] = white_pawns_attacks2_bb(wpawns);
    data.attacks2[BLACK] = black_pawns_attacks2_bb(bpawns);

    kpe->value += evaluate_backward(kpe, &data, WHITE, wpawns, bpawns);
    kpe->value -= evaluate_backward(kpe, &data, BLACK, bpawns, wpawns);

    kpe->value += evaluate_connected(wpawns, WHITE);
    kpe->value -= evaluate_connected(bpawns, BLACK);

    kpe->value += evaluate_passed(kpe, &data, WHITE, wpawns, bpawns);
    kpe->value -= evaluate_passed(kpe, &data, BLACK, bpawns, wpawns);

    kpe->value += evaluate_passed_pos(kpe, board, WHITE);
    kpe->value -= evaluate_passed_pos(kpe, board, BLACK);

    kpe->value += evaluate_doubled_isolated(wpawns, WHITE);
    kpe->value -= evaluate_doubled_isolated(bpawns, BLACK);

    return kpe;
}
