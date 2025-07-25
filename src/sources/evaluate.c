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

#include "evaluate.h"

#include <string.h>

#include "attacks.h"
#include "endgame.h"
#include "kp_eval.h"
#include "movelist.h"
#include "psq_table.h"

EvalTrace Trace;

// clang-format off

// Special eval terms
const Scorepair Initiative = SPAIR(24, 32);

// Knight eval terms
const Scorepair KnightShielded = SPAIR(  4,  23);
const Scorepair KnightOutpost  = SPAIR( 31,  30);

const Scorepair ClosedPosKnight[5] = {
    SPAIR(  6, -18), SPAIR(  7,   3), SPAIR(  8,  20), SPAIR( 13,  29),
    SPAIR( 14,  46)
};

// Bishop eval terms
const Scorepair BishopPairBonus    = SPAIR( 22,  93);
const Scorepair BishopShielded     = SPAIR(  1,   3);
const Scorepair BishopOutpost      = SPAIR( 47,  24);
const Scorepair BishopLongDiagonal = SPAIR( 13,  22);

const Scorepair BishopPawnsSameColor[7] = {
    SPAIR( 15,  36), SPAIR( 15,  26), SPAIR( 13,  17), SPAIR(  9,  11),
    SPAIR(  6,   3), SPAIR(  3,  -2), SPAIR( -3, -10)
};

// Rook eval terms
const Scorepair RookOnSemiOpenFile = SPAIR( 19,  13);
const Scorepair RookOnOpenFile     = SPAIR( 35,   9);
const Scorepair RookOnBlockedFile  = SPAIR( -8,  -8);
const Scorepair RookXrayQueen      = SPAIR( 15,   4);
const Scorepair RookTrapped        = SPAIR( -8, -16);
const Scorepair RookBuried         = SPAIR(-69, -33);

// Mobility eval terms
const Scorepair KnightMobility[9] = {
    SPAIR( -56,   20), SPAIR( -45,  -28), SPAIR( -35,   36), SPAIR( -26,   58),
    SPAIR( -19,   76), SPAIR( -13,   95), SPAIR(  -8,  104), SPAIR(   0,  110),
    SPAIR(   4,  104)
};

const Scorepair BishopMobility[14] = {
    SPAIR( -56,  -44), SPAIR( -44,  -39), SPAIR( -27,  -15), SPAIR( -25,   15),
    SPAIR( -17,   32), SPAIR( -13,   46), SPAIR( -10,   56), SPAIR(  -8,   61),
    SPAIR(  -7,   64), SPAIR(  -5,   67), SPAIR(  -2,   61), SPAIR(   7,   56),
    SPAIR(   9,   57), SPAIR(  34,   49)
};

const Scorepair RookMobility[15] = {
    SPAIR( -88,  -47), SPAIR( -39,   40), SPAIR( -27,   76), SPAIR( -31,   92),
    SPAIR( -29,  103), SPAIR( -33,  117), SPAIR( -34,  125), SPAIR( -29,  131),
    SPAIR( -26,  140), SPAIR( -17,  149), SPAIR( -17,  156), SPAIR( -13,  163),
    SPAIR(  -6,  167), SPAIR(   4,  169), SPAIR(  23,  161)
};

const Scorepair QueenMobility[28] = {
    SPAIR( -27, -114), SPAIR(  19,  212), SPAIR(   0,  148), SPAIR(  -6,   98),
    SPAIR(   1,   66), SPAIR(  -2,  118), SPAIR(  -4,  151), SPAIR(  -3,  175),
    SPAIR(  -2,  185), SPAIR(  -2,  205), SPAIR(  -1,  214), SPAIR(   2,  219),
    SPAIR(   2,  228), SPAIR(   5,  232), SPAIR(   5,  237), SPAIR(   5,  243),
    SPAIR(   6,  244), SPAIR(   9,  241), SPAIR(  14,  232), SPAIR(  12,  234),
    SPAIR(  37,  204), SPAIR(  39,  198), SPAIR(  47,  184), SPAIR(  32,  164),
    SPAIR(  57,  158), SPAIR(   6,  166), SPAIR(  14,  175), SPAIR(  40,  148)
};

// Passed Pawn eval terms
const Scorepair PassedBlocked[4] = {
    SPAIR(  -2,  -29),
    SPAIR(   2,  -52),
    SPAIR(   3,  -89),
    SPAIR( -44, -131)
};

const Scorepair PassedSafeAdvance[4] = {
    SPAIR(  -3,   26),
    SPAIR( -10,   45),
    SPAIR(   8,   70),
    SPAIR(  51,   61)
};

// King Safety linear eval terms
const Scorepair FarKnight = SPAIR(-22, -13);
const Scorepair FarBishop = SPAIR( -8, -10);
const Scorepair FarRook   = SPAIR(-10,   5);
const Scorepair FarQueen  = SPAIR( -8,  15);

// King Safety eval terms
const Scorepair KnightWeight    = SPAIR(  46,   62);
const Scorepair BishopWeight    = SPAIR(  31,  106);
const Scorepair RookWeight      = SPAIR(  33,  -46);
const Scorepair QueenWeight     = SPAIR(  10,    1);
const Scorepair AttackWeight    = SPAIR(   9,   35);
const Scorepair WeakKingZone    = SPAIR(  27,  -75);
const Scorepair SafeKnightCheck = SPAIR(  74,   31);
const Scorepair SafeBishopCheck = SPAIR(  36,  131);
const Scorepair SafeRookCheck   = SPAIR(  90,  156);
const Scorepair SafeQueenCheck  = SPAIR(  45,  207);
const Scorepair UnsafeCheck     = SPAIR(  16,  119);
const Scorepair QueenlessAttack = SPAIR( -87,  -66);
const Scorepair SafetyOffset    = SPAIR(  18,   50);

// Storm/Shelter indexes:
// 0-7 - Side
// 9-15 - Front
// 16-23 - Center
const Scorepair KingStorm[24] = {
    SPAIR(  -2,   -5), SPAIR( -34,   -1), SPAIR(  24,   15), SPAIR(   7,   14),
    SPAIR( -11,   22), SPAIR( -13,    8), SPAIR( -33,  -10), SPAIR(  -4,  -29),
    SPAIR(   0,    0), SPAIR(   2,   -7), SPAIR(  33,   28), SPAIR(   3,  -11),
    SPAIR(  -6,  -15), SPAIR(  -7,   16), SPAIR(   3,   49), SPAIR(  15,  -10),
    SPAIR(   9,   -1), SPAIR(  13,    0), SPAIR(  34,    9), SPAIR(  21,   -9),
    SPAIR(  -9,  -18), SPAIR( -15,   40), SPAIR(  -6,   97), SPAIR(  -2,  -71)
};

const Scorepair KingShelter[24] = {
    SPAIR( -38,   16), SPAIR( -27,  131), SPAIR( -26,  -43), SPAIR( -12,   11),
    SPAIR(  19,   -7), SPAIR(  18,   -6), SPAIR( -16,    0), SPAIR(   6,  -86),
    SPAIR(   0,    0), SPAIR(  -8,  -34), SPAIR(  -8,   76), SPAIR(   0,   49),
    SPAIR(   9,   23), SPAIR(  35,    0), SPAIR(   3,    0), SPAIR(  13,  -62),
    SPAIR( -37,  -49), SPAIR(  12, -150), SPAIR(  -1,   89), SPAIR(   7,  128),
    SPAIR(  10,   28), SPAIR(  21,    0), SPAIR(  15,    0), SPAIR(  13,    1)
};

// Threat eval terms
const Scorepair PawnThreats[6] = {
    SPAIR( -2, -31), SPAIR( 71,  65), SPAIR( 67, 111),
    SPAIR( 61,  64), SPAIR( 62,  23), 0
};

const Scorepair KnightThreats[6] = {
    SPAIR( -9,   8), SPAIR(  4,  40), SPAIR( 42,  56),
    SPAIR( 93,  46), SPAIR( 49,  33), 0
};

const Scorepair BishopThreats[6] = {
    SPAIR( -3,   5), SPAIR( 15,  50), SPAIR(  2,  50),
    SPAIR( 55,  75), SPAIR( 51, 148), 0
};

const Scorepair RookThreats[6] = {
    SPAIR(-10,  14), SPAIR(  7,  29), SPAIR( 24,  22),
    SPAIR( 12,  23), SPAIR( 50,  63), 0
};

const Scorepair QueenThreats[6] = {
    SPAIR(  0,   9), SPAIR(  1,  15), SPAIR( -4,  39),
    SPAIR( -2,  -4), SPAIR(  9,   2), 0
};

const Scorepair HangingPawn = SPAIR( 13,  52);

// clang-format on

static bool opposite_colored_bishops(Bitboard bishops) {
    return (bishops & DSQ_BB) && (bishops & LSQ_BB);
}

static bool is_kxk_endgame(const Board *board, Color us) {
    // If the weak side has pieces or Pawns, this is not a KXK endgame.
    if (bb_more_than_one(board_color_bb(board, color_flip(us)))) {
        return false;
    }

    return board->stack->material[us] >= ROOK_MG_SCORE;
}

static Score eval_kxk(const Board *board, Color us) {
    // Be careful to avoid stalemating the weak King.
    if (board->side_to_move != us && !board->stack->checkers) {
        Movelist list;

        movelist_generate_legal(&list, board);

        if (movelist_size(&list) == 0) {
            return 0;
        }
    }

    const Square winning_ksq = board_king_square(board, us);
    const Square losing_ksq = board_king_square(board, color_flip(us));
    Score score = board->stack->material[us]
        + board_piece_count(board, create_piece(us, PAWN)) * PAWN_MG_SCORE;

    // Push the weak King to the corner.
    score += corner_bonus(losing_ksq);

    // Keep the two Kings close for mating with low material.
    score += close_bonus(winning_ksq, losing_ksq);

    // Set the score as winning if we have mating material:
    // - a major piece;
    // - a Bishop and a Knight;
    // - two opposite colored Bishops;
    // - three Knights.
    // Note that the KBNK case has already been handled at this point
    // in the eval, so it's not necessary to worry about it.
    const Bitboard knights = board_piecetype_bb(board, KNIGHT);
    const Bitboard bishops = board_piecetype_bb(board, BISHOP);

    if (board_piecetypes_bb(board, QUEEN, ROOK) || (knights && bishops)
        || opposite_colored_bishops(bishops) || bb_popcount(knights) >= 3) {
        score += VICTORY;
    }

    return (board->side_to_move == us) ? score : -score;
}

static void evaldata_init(EvaluationData *evaldata, const Board *board, Color us) {
    const Color them = color_flip(us);
    const Square our_king = board_king_square(board, us);
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard our_pawn_attacks = pawns_attacks_bb(our_pawns, us);

    trace_add(
        IDX_PSQT + 48 + (KING - KNIGHT) * 32 + square_to_psqt_index(square_relative(our_king, us)),
        us,
        1
    );

    // Set the King Attack zone as the 3x4 square surrounding the King (counting an additional rank
    // in front of the King). Initialize the attack tables at the same time.
    evaldata->attacked[us] = evaldata->attacked_by[us][KING] = evaldata->king_zone[them] =
        king_attacks_bb(our_king);
    evaldata->king_zone[them] |= bb_shift_up_relative(evaldata->king_zone[them], us);
    if (square_file(our_king) == FILE_A) {
        evaldata->king_zone[them] |= bb_shift_right(evaldata->king_zone[them]);
    }
    if (square_file(our_king) == FILE_H) {
        evaldata->king_zone[them] |= bb_shift_left(evaldata->king_zone[them]);
    }

    // Initialize pawn attacks.
    evaldata->attacked_by[us][PAWN] = our_pawn_attacks;
    evaldata->attacked2[us] |= evaldata->attacked[us] & our_pawn_attacks;
    evaldata->attacked2[us] |= pawns_attacks2_bb(our_pawns, us);
    evaldata->attacked[us] |= our_pawn_attacks;

    // Set the initial opponent's mobility zone as the squares not attacked by our pawns.
    // Additionally, exclude those squares from the King safety zone.
    evaldata->mobility_zone[them] = ~our_pawn_attacks;
    evaldata->king_zone[them] &= ~our_pawn_attacks;
}

static void evaldata_init_next(EvaluationData *evaldata, const Board *board, Color us) {
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard occupancy = board_occupancy_bb(board);
    const Bitboard low_ranks = (us == WHITE) ? RANK_2_BB | RANK_3_BB : RANK_6_BB | RANK_7_BB;

    // Exclude our King, rammed pawns and pawns on low ranks from the mobility zone.
    evaldata->mobility_zone[us] &=
        ~(our_pawns & (bb_shift_down_relative(occupancy, us) | low_ranks));
    evaldata->mobility_zone[us] &= ~board_piece_bb(board, us, KING);
}

static void evaldata_set_position_closed(EvaluationData *evaldata, const Board *board) {
    const Bitboard wpawns = board_piece_bb(board, WHITE, PAWN);
    const Bitboard bpawns = board_piece_bb(board, BLACK, PAWN);
    const Bitboard occupancy = board_occupancy_bb(board);

    // Compute position closedness based on the number of pawns who cannot advance (including pawns
    // with their stop squares controlled).
    const Bitboard fixed_pawns =
        (wpawns & bb_shift_down(occupancy | evaldata->attacked_by[BLACK][PAWN]))
        | (bpawns & bb_shift_up(occupancy | evaldata->attacked_by[WHITE][PAWN]));

    evaldata->position_closed = i32_min(4, bb_popcount(fixed_pawns) / 2);
}

static Scorepair evaluate_knights(
    const Board *restrict board,
    EvaluationData *restrict evaldata,
    const KingPawnEntry *kpe,
    Color us
) {
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard outpost_ranks = RANK_4_BB | RANK_5_BB | (us == WHITE ? RANK_6_BB : RANK_3_BB);
    Scorepair ret = 0;
    Bitboard bb = board_piece_bb(board, us, KNIGHT);

    while (bb) {
        const Square sq = bb_pop_first_square(&bb);
        Bitboard b = knight_attacks_bb(sq);

        trace_add(IDX_PIECE + KNIGHT - PAWN, us, 1);
        trace_add(
            IDX_PSQT + 48 + (KNIGHT - KNIGHT) * 32 + square_to_psqt_index(square_relative(sq, us)),
            us,
            1
        );

        // Give a bonus/penalty based on how closed the position is.
        ret += ClosedPosKnight[evaldata->position_closed];
        trace_add(IDX_KNIGHT_CLOSED_POS + evaldata->position_closed, us, 1);

        // If the Knight is pinned, it has no Mobility squares.
        if (bb_square_is_set(board->stack->king_blockers[us], sq)) {
            b = 0;
        }

        // Update the attack tables.
        evaldata->attacked_by[us][KNIGHT] |= b;
        evaldata->attacked2[us] |= evaldata->attacked[us] & b;
        evaldata->attacked[us] |= b;

        // Give a bonus for the Knight's mobility.
        ret += KnightMobility[bb_popcount(b & evaldata->mobility_zone[us])];
        trace_add(IDX_MOBILITY_KNIGHT + bb_popcount(b & evaldata->mobility_zone[us]), us, 1);

        // Give a bonus for a Knight with a Pawn above it.
        if (bb_square_is_set(bb_shift_down_relative(our_pawns, us), sq)) {
            ret += KnightShielded;
            trace_add(IDX_KNIGHT_SHIELDED, us, 1);
        }

        // Give a bonus for a Knight on an outpost.
        if (bb_square_is_set(
                outpost_ranks & evaldata->attacked_by[us][PAWN] & ~kpe->attack_span[color_flip(us)],
                sq
            )) {
            ret += KnightOutpost;
            trace_add(IDX_KNIGHT_OUTPOST, us, 1);
        }

        // Give a penalty for a Knight not defending our King.
        if (square_distance(sq, board_king_square(board, us)) > 3) {
            ret += FarKnight;
            trace_add(IDX_FAR_KNIGHT, us, 1);
        }

        // Give a bonus for a Knight targeting the King Attack zone.
        if (b & evaldata->king_zone[us]) {
            evaldata->safety_attackers[us] += 1;
            evaldata->safety_attacks[us] += bb_popcount(b & evaldata->king_zone[us]);
            evaldata->safety_value[us] += KnightWeight;
            trace_add(IDX_KS_KNIGHT, us, 1);
            trace_add(IDX_KS_ATTACK, us, bb_popcount(b & evaldata->king_zone[us]));
        }
    }

    return ret;
}

static Scorepair evaluate_bishops(
    const Board *restrict board,
    EvaluationData *restrict evaldata,
    const KingPawnEntry *kpe __attribute__((unused)),
    Color us
) {
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard outpost_ranks = RANK_4_BB | RANK_5_BB | (us == WHITE ? RANK_6_BB : RANK_3_BB);
    const Bitboard occupancy = board_occupancy_bb(board) ^ board_piece_bb(board, us, QUEEN);
    Scorepair ret = 0;
    Bitboard bb = board_piece_bb(board, us, BISHOP);

    // Give a bonus for the Bishop pair.
    if (bb_more_than_one(bb)) {
        ret += BishopPairBonus;
        trace_add(IDX_BISHOP_PAIR, us, 1);
    }

    while (bb) {
        const Square sq = bb_pop_first_square(&bb);
        Bitboard b = bishop_attacks_bb(sq, occupancy);

        trace_add(IDX_PIECE + BISHOP - PAWN, us, 1);
        trace_add(
            IDX_PSQT + 48 + (BISHOP - KNIGHT) * 32 + square_to_psqt_index(square_relative(sq, us)),
            us,
            1
        );

        // Give a bonus/penalty based on the number of friendly Pawns which are
        // on same color squares as the Bishop.
        {
            const Bitboard sq_mask = bb_square_is_set(DSQ_BB, sq) ? DSQ_BB : LSQ_BB;

            ret += BishopPawnsSameColor[u8_min(bb_popcount(sq_mask & our_pawns), 6)];
            trace_add(IDX_BISHOP_PAWNS_COLOR + u8_min(bb_popcount(sq_mask & our_pawns), 6), us, 1);
        }

        // If the Bishop is pinned, reduce its mobility to all the squares between the King and the
        // pinner.
        if (bb_square_is_set(board->stack->king_blockers[us], sq)) {
            b &= line_bb(board_king_square(board, us), sq);
        }

        // Update the attack tables.
        evaldata->attacked_by[us][BISHOP] |= b;
        evaldata->attacked2[us] |= evaldata->attacked[us] & b;
        evaldata->attacked[us] |= b;

        // Give a bonus for the Bishop's mobility.
        ret += BishopMobility[bb_popcount(b & evaldata->mobility_zone[us])];
        trace_add(IDX_MOBILITY_BISHOP + bb_popcount(b & evaldata->mobility_zone[us]), us, 1);

        // Give a bonus for a Bishop with a Pawn above it.
        if (bb_square_is_set(bb_shift_down_relative(our_pawns, us), sq)) {
            ret += BishopShielded;
            trace_add(IDX_BISHOP_SHIELDED, us, 1);
        }

        // Give a bonus for a Bishop on an outpost.
        if (bb_square_is_set(
                outpost_ranks & evaldata->attacked_by[us][PAWN] & ~kpe->attack_span[color_flip(us)],
                sq
            )) {
            ret += BishopOutpost;
            trace_add(IDX_BISHOP_OUTPOST, us, 1);
        }

        // Give a bonus for a Bishop seeing the center squares (generally in
        // fianchetto).
        if (bb_more_than_one(b & CENTER_BB)) {
            ret += BishopLongDiagonal;
            trace_add(IDX_BISHOP_LONG_DIAG, us, 1);
        }

        // Give a penalty for a Bishop not defending our King.
        if (square_distance(sq, board_king_square(board, us)) > 3) {
            ret += FarBishop;
            trace_add(IDX_FAR_BISHOP, us, 1);
        }

        // Give a bonus for a Bishop targeting the King Attack zone.
        if (b & evaldata->king_zone[us]) {
            evaldata->safety_attackers[us] += 1;
            evaldata->safety_attacks[us] += bb_popcount(b & evaldata->king_zone[us]);
            evaldata->safety_value[us] += BishopWeight;
            trace_add(IDX_KS_BISHOP, us, 1);
            trace_add(IDX_KS_ATTACK, us, bb_popcount(b & evaldata->king_zone[us]));
        }
    }

    return ret;
}

static Scorepair
    evaluate_rooks(const Board *restrict board, EvaluationData *restrict evaldata, Color us) {
    const Bitboard occupancy = board_occupancy_bb(board) ^ board_pieces_bb(board, us, ROOK, QUEEN);
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard their_pawns = board_piece_bb(board, color_flip(us), PAWN);
    Scorepair ret = 0;
    Bitboard bb = board_piece_bb(board, us, ROOK);

    while (bb) {
        const Square sq = bb_pop_first_square(&bb);
        const Bitboard rook_file_bb = square_file_bb(sq);
        Bitboard b = rook_attacks_bb(sq, occupancy);

        trace_add(IDX_PIECE + ROOK - PAWN, us, 1);
        trace_add(
            IDX_PSQT + 48 + (ROOK - KNIGHT) * 32 + square_to_psqt_index(square_relative(sq, us)),
            us,
            1
        );

        // If the Rook is pinned, reduce its mobility to all the squares between the King and the
        // pinner.
        if (bb_square_is_set(board->stack->king_blockers[us], sq)) {
            b &= line_bb(board_king_square(board, us), sq);
        }

        // Update the attack tables.
        evaldata->attacked_by[us][ROOK] |= b;
        evaldata->attacked2[us] |= evaldata->attacked[us] & b;
        evaldata->attacked[us] |= b;

        // Give a bonus for a Rook on an open (or semi-open) file.
        if (!(rook_file_bb & our_pawns)) {
            ret += (rook_file_bb & their_pawns) ? RookOnSemiOpenFile : RookOnOpenFile;
            trace_add((rook_file_bb & their_pawns) ? IDX_ROOK_SEMIOPEN : IDX_ROOK_OPEN, us, 1);
        }
        // Give a penalty for a Rook on a file with a blocked friendly Pawn.
        else if (bb_shift_up_relative(rook_file_bb & our_pawns, us) & occupancy) {
            ret += RookOnBlockedFile;
            trace_add(IDX_ROOK_BLOCKED, us, 1);
        }

        // Give a bonus for a Rook on the same file as the opponent's Queen(s).
        if (rook_file_bb & board_piece_bb(board, color_flip(us), QUEEN)) {
            ret += RookXrayQueen;
            trace_add(IDX_ROOK_XRAY_QUEEN, us, 1);
        }

        const u8 mobility = bb_popcount(b & evaldata->mobility_zone[us]);

        // Give a bonus for the Rook's mobility.
        ret += RookMobility[mobility];
        trace_add(IDX_MOBILITY_ROOK + mobility, us, 1);

        // Check for trapped Rooks.
        if (mobility <= 4 && square_rank_relative(sq, us) <= RANK_2) {
            const File king_file = square_file(board_king_square(board, us));
            const File rook_file = square_file(sq);

            // Check if the King resides between the Rook and the middle of the board.
            if (king_file != rook_file && (king_file < rook_file) == (king_file >= FILE_E)) {
                const bool can_castle = !!(board->stack->castlings & relative_clmask(us));
                ret += can_castle ? RookTrapped : RookBuried;
                trace_add(can_castle ? IDX_ROOK_TRAPPED : IDX_ROOK_BURIED, us, 1);
            }
        }

        // Give a penalty for a Rook not defending our King.
        if (square_distance(sq, board_king_square(board, us)) > 3) {
            ret += FarRook;
            trace_add(IDX_FAR_ROOK, us, 1);
        }

        // Give a bonus for a Rook targeting the King Attack zone.
        if (b & evaldata->king_zone[us]) {
            evaldata->safety_attackers[us] += 1;
            evaldata->safety_attacks[us] += bb_popcount(b & evaldata->king_zone[us]);
            evaldata->safety_value[us] += RookWeight;
            trace_add(IDX_KS_ROOK, us, 1);
            trace_add(IDX_KS_ATTACK, us, bb_popcount(b & evaldata->king_zone[us]));
        }
    }

    return ret;
}

static Scorepair
    evaluate_queens(const Board *restrict board, EvaluationData *restrict evaldata, Color us) {
    const Bitboard occupancy_b = board_occupancy_bb(board) ^ board_piece_bb(board, us, BISHOP);
    const Bitboard occupancy_r = board_occupancy_bb(board) ^ board_piece_bb(board, us, ROOK);
    Scorepair ret = 0;
    Bitboard bb = board_piece_bb(board, us, QUEEN);

    while (bb) {
        const Square sq = bb_pop_first_square(&bb);
        Bitboard b = bishop_attacks_bb(sq, occupancy_b) | rook_attacks_bb(sq, occupancy_r);

        trace_add(IDX_PIECE + QUEEN - PAWN, us, 1);
        trace_add(
            IDX_PSQT + 48 + (QUEEN - KNIGHT) * 32 + square_to_psqt_index(square_relative(sq, us)),
            us,
            1
        );

        // If the Queen is pinned, reduce its mobility to all the squares between the King and the
        // pinner.
        if (bb_square_is_set(board->stack->king_blockers[us], sq)) {
            b &= line_bb(board_king_square(board, us), sq);
        }

        // Update the attack tables.
        evaldata->attacked_by[us][QUEEN] |= b;
        evaldata->attacked2[us] |= evaldata->attacked[us] & b;
        evaldata->attacked[us] |= b;

        // Give a bonus for the Queen's mobility.
        ret += QueenMobility[bb_popcount(b & evaldata->mobility_zone[us])];
        trace_add(IDX_MOBILITY_QUEEN + bb_popcount(b & evaldata->mobility_zone[us]), us, 1);

        // Give a penalty for a Queen not defending our King.
        if (square_distance(sq, board_king_square(board, us)) > 3) {
            ret += FarQueen;
            trace_add(IDX_FAR_QUEEN, us, 1);
        }

        // Give a bonus for a Queen targeting the King Attack zone.
        if (b & evaldata->king_zone[us]) {
            evaldata->safety_attackers[us] += 1;
            evaldata->safety_attacks[us] += bb_popcount(b & evaldata->king_zone[us]);
            evaldata->safety_value[us] += QueenWeight;
            trace_add(IDX_KS_QUEEN, us, 1);
            trace_add(IDX_KS_ATTACK, us, bb_popcount(b & evaldata->king_zone[us]));
        }
    }

    return ret;
}

static Scorepair evaluate_passed(
    const Board *board,
    const EvaluationData *restrict evaldata,
    const KingPawnEntry *kpe,
    Color us
) {
    const Color them = color_flip(us);
    const Bitboard occ = board_occupancy_bb(board);
    Bitboard passed = kpe->passed[us];
    Scorepair ret = 0;

    while (passed) {
        const Square sq = bb_pop_first_square(&passed);
        const Rank rank = square_rank_relative(sq, us);

        const u8 queening_distance = RANK_8 - rank;
        if (queening_distance > 4) {
            continue;
        }

        // Give a penalty for a blocked passed pawn
        if (bb_square_is_set(bb_shift_down_relative(occ, us), sq)) {
            ret += PassedBlocked[rank - RANK_4];
            trace_add(IDX_PASSED_BLOCKED + rank - RANK_4, us, 1);
        }

        // Give a bonus for a passed pawn that can safely advance
        if (!bb_square_is_set(bb_shift_down_relative(evaldata->attacked[them], us), sq)) {
            ret += PassedSafeAdvance[rank - RANK_4];
            trace_add(IDX_PASSED_SAFE_ADVANCE + rank - RANK_4, us, 1);
        }
    }

    return ret;
}

static Scorepair evaluate_threats(const Board *board, const EvaluationData *evaldata, Color us) {
    const Color them = color_flip(us);
    const Bitboard their_pieces = board_color_bb(board, them);
    Scorepair ret = 0;
    Bitboard threats;

    threats = their_pieces & evaldata->attacked_by[us][PAWN];

    // Give a bonus for Pawn threats.
    while (threats) {
        const Piecetype threatened =
            piece_type(board_piece_on(board, bb_pop_first_square(&threats)));
        ret += PawnThreats[threatened - PAWN];
        trace_add(IDX_PAWN_THREAT + threatened - PAWN, us, 1);
    }

    threats = their_pieces & evaldata->attacked_by[us][KNIGHT];

    // Give a bonus for Knight threats.
    while (threats) {
        const Piecetype threatened =
            piece_type(board_piece_on(board, bb_pop_first_square(&threats)));
        ret += KnightThreats[threatened - PAWN];
        trace_add(IDX_KNIGHT_THREAT + threatened - PAWN, us, 1);
    }

    threats = their_pieces & evaldata->attacked_by[us][BISHOP];

    // Give a bonus for Bishop threats.
    while (threats) {
        const Piecetype threatened =
            piece_type(board_piece_on(board, bb_pop_first_square(&threats)));
        ret += BishopThreats[threatened - PAWN];
        trace_add(IDX_BISHOP_THREAT + threatened - PAWN, us, 1);
    }

    threats = their_pieces & evaldata->attacked_by[us][ROOK];

    // Give a bonus for Rook threats.
    while (threats) {
        const Piecetype threatened =
            piece_type(board_piece_on(board, bb_pop_first_square(&threats)));
        ret += RookThreats[threatened - PAWN];
        trace_add(IDX_ROOK_THREAT + threatened - PAWN, us, 1);
    }

    threats = their_pieces & evaldata->attacked_by[us][QUEEN];

    // Give a bonus for Queen threats.
    while (threats) {
        const Piecetype threatened =
            piece_type(board_piece_on(board, bb_pop_first_square(&threats)));
        ret += QueenThreats[threatened - PAWN];
        trace_add(IDX_QUEEN_THREAT + threatened - PAWN, us, 1);
    }

    threats =
        board_piece_bb(board, them, PAWN) & ~evaldata->attacked[them] & evaldata->attacked[us];

    // Give a bonus if the opponent has hanging Pawns.
    if (threats) {
        ret += HangingPawn * bb_popcount(threats);
        trace_add(IDX_HANGING_PAWN, us, bb_popcount(threats));
    }

    return ret;
}

static Scorepair evaluate_safety_file(
    Bitboard our_pawns,
    Bitboard their_pawns,
    File f,
    Square their_king,
    Color us
) {
    // Map the base array index depending on whether the current file is on the
    // side (0), in the middle of the field (16), or the same as the King (8).
    const File king_file = square_file(their_king);
    const u8 offset = (king_file == f) ? 8 : (king_file >= FILE_E) == (king_file < f) ? 0 : 16;
    Scorepair ret = 0;

    // Give a bonus/penalty based on the rank distance between our Pawns and their King.
    {
        const Bitboard file_pawns = our_pawns & file_bb(f);
        const u8 distance = file_pawns
            ? square_rank_distance(bb_last_square_relative(file_pawns, us), their_king)
            : 7;

        ret += KingStorm[offset + distance];

        trace_add(IDX_KS_STORM + offset + distance, us, 1);
    }

    // Give a bonus/penalty based on the rank distance between their Pawns and their King.
    {
        const Bitboard file_pawns = their_pawns & file_bb(f);
        const u8 distance = file_pawns
            ? square_rank_distance(bb_last_square_relative(file_pawns, us), their_king)
            : 7;

        ret += KingShelter[offset + distance];

        trace_add(IDX_KS_SHELTER + offset + distance, us, 1);
    }

    return ret;
}

static Scorepair evaluate_safety(const Board *board, const EvaluationData *evaldata, Color us) {
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone, or
    // one piece attacking with a friendly Queen still on the board.
    const bool queenless = !board_piece_bb(board, us, QUEEN);

    if (evaldata->safety_attackers[us] < 1 + queenless) {
        trace_clear_safety(us);
        return 0;
    }

    const Color them = color_flip(us);
    const Square their_king = board_king_square(board, them);

    // We define weak squares as squares that we attack where the enemy has
    // no defenders, or only the King as a defender.
    const Bitboard weak_squares = evaldata->attacked[us] & ~evaldata->attacked2[them]
        & (~evaldata->attacked[them] | evaldata->attacked_by[them][KING]);

    // We define safe squares as squares that are attacked (or weak and attacked twice),
    // and where we can land on.
    const Bitboard safe_squares = ~board_color_bb(board, us)
        & (~evaldata->attacked[them] | (weak_squares & evaldata->attacked2[us]));

    const Bitboard rook_check_span = rook_attacks_bb(their_king, board_occupancy_bb(board));
    const Bitboard bishop_check_span = bishop_attacks_bb(their_king, board_occupancy_bb(board));

    const Bitboard knight_checks =
        evaldata->attacked_by[us][KNIGHT] & knight_attacks_bb(their_king);
    const Bitboard bishop_checks = evaldata->attacked_by[us][BISHOP] & bishop_check_span;
    const Bitboard rook_checks = evaldata->attacked_by[us][ROOK] & rook_check_span;
    const Bitboard queen_checks =
        evaldata->attacked_by[us][QUEEN] & (bishop_check_span | rook_check_span);
    const Bitboard all_checks = knight_checks | bishop_checks | rook_checks | queen_checks;

    Scorepair bonus = evaldata->safety_value[us] + SafetyOffset;

    // Add bonuses based on the number of attacks and the number of weak
    // squares in the King zone, and a penalty if we don't have a Queen for
    // attacking.
    bonus += AttackWeight * evaldata->safety_attacks[us];
    bonus += WeakKingZone * bb_popcount(weak_squares & evaldata->king_zone[us]);
    bonus += QueenlessAttack * queenless;

    // Add bonuses per piece for each safe check that we can perform.
    bonus += SafeKnightCheck * bb_popcount(knight_checks & safe_squares);
    bonus += SafeBishopCheck * bb_popcount(bishop_checks & safe_squares);
    bonus += SafeRookCheck * bb_popcount(rook_checks & safe_squares);
    bonus += SafeQueenCheck * bb_popcount(queen_checks & safe_squares);

    // Add a bonus for all unsafe checks we can perform.
    bonus += UnsafeCheck * bb_popcount(all_checks & ~safe_squares);

    // Evaluate the Pawn Storm/Shelter.
    {
        const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
        const Bitboard their_pawns = board_piece_bb(board, them, PAWN);
        const File king_file = square_file(their_king);

        if (file_is_valid(king_file - 1)) {
            bonus += evaluate_safety_file(our_pawns, their_pawns, king_file - 1, their_king, us);
        }

        bonus += evaluate_safety_file(our_pawns, their_pawns, king_file, their_king, us);

        if (file_is_valid(king_file + 1)) {
            bonus += evaluate_safety_file(our_pawns, their_pawns, king_file + 1, their_king, us);
        }
    }

    trace_add(IDX_KS_OFFSET, us, 1);
    trace_add(IDX_KS_QUEENLESS, us, queenless);
    trace_add(IDX_KS_WEAK_Z, us, bb_popcount(weak_squares & evaldata->king_zone[us]));
    trace_add(IDX_KS_CHECK_N, us, bb_popcount(knight_checks & safe_squares));
    trace_add(IDX_KS_CHECK_B, us, bb_popcount(bishop_checks & safe_squares));
    trace_add(IDX_KS_CHECK_R, us, bb_popcount(rook_checks & safe_squares));
    trace_add(IDX_KS_CHECK_Q, us, bb_popcount(queen_checks & safe_squares));
    trace_add(IDX_KS_UNSAFE_CHECK, us, bb_popcount(all_checks & ~safe_squares));
    trace_set_safety(us, bonus);

    // Compute the final safety score. The middlegame term scales
    // quadratically, while the endgame term scales linearly. The safety
    // evaluation cannot be negative.
    const Score mg = scorepair_midgame(bonus);
    const Score eg = scorepair_endgame(bonus);
    return create_scorepair(i32_max(mg, 0) * mg / 256, i16_max(eg, 0) / 16);
}

static bool eval_is_ocb_endgame(const Board *board) {
    // Check if there is exactly one White Bishop and one Black Bishop.
    if (board_piece_count(board, WHITE_BISHOP) != 1
        || board_piece_count(board, BLACK_BISHOP) != 1) {
        return false;
    }

    // Then check that the Bishops are on opposite colored squares.
    return opposite_colored_bishops(board_piecetype_bb(board, BISHOP));
}

static Score eval_scale_endgame(const Board *board, const KingPawnEntry *kpe, Score eg) {
    // Only detect endgame scaling from the side with a positive evaluation. This allows us to
    // quickly filter out positions which shouldn't be scaled, even though they have a theoretical
    // scaling factor in our code (like KNvKPPPP).
    const Color strong_side = eg > 0 ? WHITE : BLACK;
    const Color weak_side = color_flip(strong_side);
    const Score strong_material = board->stack->material[strong_side];
    const Score weak_material = board->stack->material[weak_side];
    const Bitboard strong_pawns = board_piece_bb(board, strong_side, PAWN);
    const Bitboard weak_pawns = board_piece_bb(board, weak_side, PAWN);
    const EndgameEntry *entry;
    Scalefactor factor;

    // Scale down endgames with no pawns and low material difference.
    if (!strong_pawns && strong_material - weak_material <= BISHOP_MG_SCORE) {
        factor = (strong_material <= BISHOP_MG_SCORE)
            ? SCALE_DRAW
            : i16_max((i32)(strong_material - weak_material) * 8 / BISHOP_MG_SCORE, SCALE_DRAW);
    }
    // Scale down OCB endgames.
    else if (eval_is_ocb_endgame(board)) {
        factor = (strong_material + weak_material > 2 * BISHOP_MG_SCORE)
            ? 71 + (i32)board_color_count(board, strong_side) * 9
            : 33 + (i32)bb_popcount(kpe->passed[strong_side]) * 21;
    }
    // Rook endgames: drawish if the pawn advantage is small, and all strong side pawns are on the
    // same side of the board. Don't scale if the defending King is far from his own pawns.
    else if (strong_material == ROOK_MG_SCORE && weak_material == ROOK_MG_SCORE && (bb_popcount(strong_pawns) < 2 + bb_popcount(weak_pawns)) && !!(KINGSIDE_BB & strong_pawns) != !!(QUEENSIDE_BB & strong_pawns) && !!(king_attacks_bb(board_king_square(board, weak_side)) & weak_pawns)) {
        factor = 130;
    }
    // Check if we have a specialized function for the given material distribution.
    else if ((entry = endgame_probe_scale(board)) != NULL) {
        factor = entry->scale_fn(board, strong_side);
    }
    // Other endgames.
    else {
        factor = i16_min(177 + 13 * bb_popcount(strong_pawns), SCALE_NORMAL);
    }

    trace_set_scalefactor(factor);
    return (Score)((i32)eg * factor / SCALE_NORMAL);
}

Score evaluate(const Board *board) {
    trace_init();

    // Do we have a specialized endgame eval for the current configuration ?
    const EndgameEntry *entry = endgame_probe_score(board);

    if (entry != NULL) {
        return entry->score_fn(board, entry->strong_side);
    }

    // Is there a KXK situation ? (lone King vs mating material)
    if (is_kxk_endgame(board, WHITE)) {
        return eval_kxk(board, WHITE);
    }

    if (is_kxk_endgame(board, BLACK)) {
        return eval_kxk(board, BLACK);
    }

    EvaluationData evaldata;
    Scorepair tapered = board->psq_scorepair;
    KingPawnEntry *kpe;

    // Initialize the evaldata structure.
    memset(&evaldata, 0, sizeof(evaldata));
    evaldata_init(&evaldata, board, WHITE);
    evaldata_init(&evaldata, board, BLACK);
    evaldata_init_next(&evaldata, board, WHITE);
    evaldata_init_next(&evaldata, board, BLACK);
    evaldata_set_position_closed(&evaldata, board);

    // Add the King-Pawn structure evaluation.
    kpe = king_pawn_probe(board);
    tapered += kpe->value;

    // Add the pieces' evaluation.
    tapered += evaluate_knights(board, &evaldata, kpe, WHITE);
    tapered -= evaluate_knights(board, &evaldata, kpe, BLACK);
    tapered += evaluate_bishops(board, &evaldata, kpe, WHITE);
    tapered -= evaluate_bishops(board, &evaldata, kpe, BLACK);
    tapered += evaluate_rooks(board, &evaldata, WHITE);
    tapered -= evaluate_rooks(board, &evaldata, BLACK);
    tapered += evaluate_queens(board, &evaldata, WHITE);
    tapered -= evaluate_queens(board, &evaldata, BLACK);

    // Add the passed pawn evaluation
    tapered += evaluate_passed(board, &evaldata, kpe, WHITE);
    tapered -= evaluate_passed(board, &evaldata, kpe, BLACK);

    // Add the threats' evaluation.
    tapered += evaluate_threats(board, &evaldata, WHITE);
    tapered -= evaluate_threats(board, &evaldata, BLACK);

    // Add the King Safety evaluation.
    tapered += evaluate_safety(board, &evaldata, WHITE);
    tapered -= evaluate_safety(board, &evaldata, BLACK);

    // Add the Initiative bonus for the side to move.
    tapered += (board->side_to_move == WHITE) ? Initiative : -Initiative;
    trace_add(IDX_INITIATIVE, board->side_to_move, 1);

    Score mg = scorepair_midgame(tapered);
    Score eg = eval_scale_endgame(board, kpe, scorepair_endgame(tapered));
    Score score;

    trace_set_eval(tapered);

    // Compute the evaluation by interpolating between the middlegame and endgame scores.
    {
        i16 phase = 4 * board_piecetype_count(board, QUEEN) + 2 * board_piecetype_count(board, ROOK)
            + board_piecetype_count(board, BISHOP) + board_piecetype_count(board, KNIGHT);

        phase = i16_clamp(phase, ENDGAME_COUNT, MIDGAME_COUNT);
        trace_set_phase(phase);
        score = mg * (phase - ENDGAME_COUNT) / (MIDGAME_COUNT - ENDGAME_COUNT);
        score += eg * (MIDGAME_COUNT - phase) / (MIDGAME_COUNT - ENDGAME_COUNT);
    }

    // Return the score relative to the side to move.
    return board->side_to_move == WHITE ? score : -score;
}
