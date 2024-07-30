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

#include "new_evaluate.h"

#include "new_endgame.h"
#include "new_movelist.h"
#include "new_psq_table.h"

EvalTrace Trace;

// clang-format off

// Special eval terms
const Scorepair Initiative = SPAIR(24, 30);

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

Score evaluate(const Board *board) {
    trace_init();

    Scorepair tapered = board->psq_scorepair;

    // Is there a KXK situation ? (lone King vs mating material)
    if (is_kxk_endgame(board, WHITE)) {
        return eval_kxk(board, WHITE);
    }

    if (is_kxk_endgame(board, BLACK)) {
        return eval_kxk(board, BLACK);
    }

    // Add the Initiative bonus for the side to move.
    tapered += (board->side_to_move == WHITE) ? Initiative : -Initiative;
    trace_add(IDX_INITIATIVE, board->side_to_move, 1);

    Score mg = scorepair_midgame(tapered);
    Score eg = scorepair_endgame(tapered);
    Score score;

    trace_set_eval(tapered);
    trace_set_scalefactor(SCALE_NORMAL);

    // Compute the evaluation by interpolating between the middlegame and
    // endgame scores.
    {
        i16 phase = 4 * board_piecetype_count(board, QUEEN) + 2 * board_piecetype_count(board, ROOK)
            + board_piecetype_count(board, BISHOP) + board_piecetype_count(board, KNIGHT);

        phase = i16_clamp(phase, ENDGAME_COUNT, MIDGAME_COUNT);

        trace_set_phase(phase);

        score = mg * (phase - ENDGAME_COUNT) / (MIDGAME_COUNT - ENDGAME_COUNT);
        score += eg * (MIDGAME_COUNT - phase) / (MIDGAME_COUNT - ENDGAME_COUNT);
    }

    return board->side_to_move == WHITE ? score : -score;
}
