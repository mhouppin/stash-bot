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

#include "endgame.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "kp_eval.h"
#include "kpk_bitbase.h"
#include "psq_table.h"
#include "strmanip.h"
#include "strview.h"
#include "syncio.h"

static EndgameEntry EndgameTable[ENDGAME_TABLE_SIZE];

static EndgameEntry *
    material_to_eg_entry(StringView white_pieces, StringView black_pieces, Key *material_key) {
    assert(white_pieces.size <= 8 && black_pieces.size <= 8);
    String fen;
    u8 bcopy[8];
    Board board;
    Boardstack stack;

    string_init(&fen);
    string_reserve(&fen, 64);

    // Change the casing of the black pieces for the FEN representation.
    for (usize i = 0; i < black_pieces.size; ++i) {
        assert(isalpha(black_pieces.data[i]));
        bcopy[i] = black_pieces.data[i] ^ 0b00100000;
    }

    // Generate a FEN corresponding to the material distribution of the pieces.
    string_push_back_strview(&fen, STATIC_STRVIEW("8/"));
    string_push_back_range(&fen, bcopy, black_pieces.size);
    string_push_back_u64(&fen, 8 - black_pieces.size);
    string_push_back_strview(&fen, STATIC_STRVIEW("/8/8/8/8/"));
    string_push_back_strview(&fen, white_pieces);
    string_push_back_u64(&fen, 8 - white_pieces.size);
    string_push_back_strview(&fen, STATIC_STRVIEW("/8 w - - 0 1"));

    if (!board_try_init(&board, strview_from_string(&fen), false, &stack)) {
        fwrite_strview(stderr, STATIC_STRVIEW("Unable to initialize board material\n"));
        exit(EXIT_FAILURE);
    }

    *material_key = stack.material_key;
    return &EndgameTable[*material_key % ENDGAME_TABLE_SIZE];
}

static void add_scoring_colored_entry(
    StringView white_pieces,
    StringView black_pieces,
    Color strong_side,
    EndgameScoreFn score_fn
) {
    Key material_key;
    EndgameEntry *entry = material_to_eg_entry(white_pieces, black_pieces, &material_key);

    // Check for key collisions and stop the program here.
    if (entry->score_fn != NULL || (entry->key && entry->key != material_key)) {
        fwrite_strview(stderr, STATIC_STRVIEW("Error: key conflicts in endgame table\n"));
        exit(EXIT_FAILURE);
    }

    // Assign the endgame specifications to the entry.
    entry->key = material_key;
    entry->score_fn = score_fn;
    entry->strong_side = strong_side;
}

static void add_scoring_entry(StringView non_pawn_material, EndgameScoreFn score_fn) {
    const usize split_index = strview_find(non_pawn_material, 'v');
    const StringView strong_pieces = strview_subview(non_pawn_material, 0, split_index);
    const StringView weak_pieces =
        strview_subview(non_pawn_material, split_index + 1, non_pawn_material.size);

    assert(split_index != NPOS);
    add_scoring_colored_entry(strong_pieces, weak_pieces, WHITE, score_fn);

    // Add the endgame entry for both sides if the material distribution isn't identical.
    if (!strview_equals_strview(strong_pieces, weak_pieces)) {
        add_scoring_colored_entry(weak_pieces, strong_pieces, BLACK, score_fn);
    }
}

static void add_scaling_colored_entry(
    StringView white_pieces,
    StringView black_pieces,
    Color strong_side,
    EndgameScaleFn scale_fn
) {
    Key material_key;
    EndgameEntry *entry = material_to_eg_entry(white_pieces, black_pieces, &material_key);

    // Check for key collisions and stop the program here.
    if (entry->scale_fn != NULL || (entry->key && entry->key != material_key)) {
        fwrite_strview(stderr, STATIC_STRVIEW("Error: key conflicts in endgame table\n"));
        exit(EXIT_FAILURE);
    }

    // Assign the endgame specifications to the entry.
    entry->key = material_key;
    entry->scale_fn = scale_fn;
    entry->strong_side = strong_side;
}

static void add_scaling_entry(StringView non_pawn_material, EndgameScaleFn scale_fn) {
    const usize split_index = strview_find(non_pawn_material, 'v');
    const StringView strong_pieces = strview_subview(non_pawn_material, 0, split_index);
    const StringView weak_pieces =
        strview_subview(non_pawn_material, split_index + 1, non_pawn_material.size);

    assert(split_index != NPOS);
    add_scaling_colored_entry(strong_pieces, weak_pieces, WHITE, scale_fn);

    // Add the endgame entry for both sides if the material distribution isn't identical.
    if (!strview_equals_strview(strong_pieces, weak_pieces)) {
        add_scaling_colored_entry(weak_pieces, strong_pieces, BLACK, scale_fn);
    }
}

void endgame_table_init(void) {
    static const StringView drawn_endgames[] = {
        STATIC_STRVIEW("KvK"),
        STATIC_STRVIEW("KNvK"),
        STATIC_STRVIEW("KBvK"),
        STATIC_STRVIEW("KNNvK"),
        STATIC_STRVIEW("KBvKN"),
        STATIC_STRVIEW("KNvKN"),
        STATIC_STRVIEW("KBvKB"),
        STATIC_STRVIEW("KBBvKB"),
        EmptyStrview,
    };

    memset(EndgameTable, 0, sizeof(EndgameTable));

    for (usize i = 0; drawn_endgames[i].size != 0; ++i) {
        add_scoring_entry(drawn_endgames[i], &eval_draw);
    }

    // 3-man endgames
    add_scoring_entry(STATIC_STRVIEW("KPvK"), &eval_kpk);

    // 4-man endgames
    add_scoring_entry(STATIC_STRVIEW("KBNvK"), &eval_kbnk);
    add_scoring_entry(STATIC_STRVIEW("KRvKP"), &eval_krkp);
    add_scoring_entry(STATIC_STRVIEW("KRvKN"), &eval_krkn);
    add_scoring_entry(STATIC_STRVIEW("KRvKB"), &eval_krkb);
    add_scoring_entry(STATIC_STRVIEW("KQvKP"), &eval_kqkp);
    add_scoring_entry(STATIC_STRVIEW("KQvKR"), &eval_kqkr);

    // 5-man endgames
    add_scoring_entry(STATIC_STRVIEW("KNPvKN"), &eval_kmpkn);
    add_scoring_entry(STATIC_STRVIEW("KNPvKB"), &eval_kmpkb);
    add_scoring_entry(STATIC_STRVIEW("KNNvKP"), &eval_knnkp);
    add_scoring_entry(STATIC_STRVIEW("KBPvKN"), &eval_kmpkn);
    add_scoring_entry(STATIC_STRVIEW("KBPvKB"), &eval_kmpkb);
    add_scoring_entry(STATIC_STRVIEW("KRPvKR"), &eval_krpkr);

    // Scaled endgames
    add_scaling_entry(STATIC_STRVIEW("KvK"), &scale_kpsk);
    add_scaling_entry(STATIC_STRVIEW("KBvK"), &scale_kbpsk);
}

Score eval_draw(
    const Board *board __attribute__((unused)),
    Color strong_side __attribute__((unused))
) {
    return DRAW;
}

Score eval_kpk(const Board *board, Color strong_side) {
    Square strong_king = board_king_square(board, strong_side);
    Square strong_pawn = bb_first_square(board_piecetype_bb(board, PAWN));
    Square weak_king = board_king_square(board, color_flip(strong_side));
    const Color us = strong_side == board->side_to_move ? WHITE : BLACK;
    const bool flip_file = square_file(strong_pawn) >= FILE_E;

    strong_king = normalize_square(strong_side, strong_king, flip_file);
    strong_pawn = normalize_square(strong_side, strong_pawn, flip_file);
    weak_king = normalize_square(strong_side, weak_king, flip_file);

    // Probe the KPK bitbase to evaluate the position.
    const Score score = kpk_bitbase_is_winning(weak_king, strong_king, strong_pawn, us)
        ? VICTORY + PAWN_EG_SCORE + square_rank(strong_pawn) * 3
        : DRAW;
    return us == WHITE ? score : -score;
}

Score eval_kbnk(const Board *board, Color strong_side) {
    const Square strong_king = board_king_square(board, strong_side);
    Square weak_king = board_king_square(board, color_flip(strong_side));
    Score score = VICTORY + KNIGHT_MG_SCORE + BISHOP_MG_SCORE + close_bonus(weak_king, strong_king);

    // Don't push the King to the wrong corner.
    if (board_piecetype_bb(board, BISHOP) & DSQ_BB) {
        weak_king = square_flip(weak_king);
    }

    score += i8_abs((i8)square_file(weak_king) - (i8)square_rank(weak_king)) * 100;
    return board->side_to_move == strong_side ? score : -score;
}

Score eval_krkp(const Board *board, Color strong_side) {
    const Color weak_side = color_flip(strong_side);
    const Square strong_king = board_king_square(board, strong_side);
    const Square strong_rook = bb_first_square(board_piecetype_bb(board, ROOK));
    const Square weak_king = board_king_square(board, weak_side);
    const Square weak_pawn = bb_first_square(board_piecetype_bb(board, PAWN));
    const Square push_square = weak_pawn + pawn_direction(weak_side);
    const Square promotion_square =
        create_square(square_file(weak_pawn), rank_relative(RANK_8, weak_side));
    const bool strong_tempo = board->side_to_move == strong_side;
    Score score;

    // Does the strong King control the promotion path ?
    if (bb_square_is_set(forward_file_bb(strong_king, strong_side), weak_pawn)) {
        score = ROOK_EG_SCORE - square_distance(strong_king, weak_pawn);
    }
    // Is the weak King unable to reach either the Rook or its Pawn ?
    else if (square_distance(weak_king, weak_pawn) >= 3 + !strong_tempo && square_distance(weak_king, strong_rook) >= 3) {
        score = ROOK_EG_SCORE - square_distance(strong_king, weak_pawn);
    }
    // Is the Pawn close to the promotion square and defended by its King ? Also, is the strong King
    // unable to reach the Pawn ?
    else if (square_rank_relative(weak_king, strong_side) <= RANK_3 && square_distance(weak_king, weak_pawn) == 1 && square_rank_relative(strong_king, strong_side) >= RANK_4 && square_distance(strong_king, weak_pawn) >= 3 + strong_tempo) {
        score = 40 - 4 * square_distance(strong_king, weak_pawn);
    } else {
        score = 100
            - 4
                * (square_distance(strong_king, push_square)
                   - square_distance(weak_king, push_square)
                   - square_distance(weak_pawn, promotion_square));
    }

    return strong_tempo ? score : -score;
}

Score eval_krkn(const Board *board, Color strong_side) {
    const Square weak_king = board_king_square(board, color_flip(strong_side));
    const Square weak_knight = bb_first_square(board_piecetype_bb(board, KNIGHT));
    const Score score = corner_bonus(weak_king) + away_bonus(weak_king, weak_knight);
    return board->side_to_move == strong_side ? score : -score;
}

Score eval_krkb(const Board *board, Color strong_side) {
    const Score score = corner_bonus(board_king_square(board, color_flip(strong_side)));
    return board->side_to_move == strong_side ? score : -score;
}

Score eval_kqkp(const Board *board, Color strong_side) {
    const Color weak_side = color_flip(strong_side);
    const Square strong_king = board_king_square(board, strong_side);
    const Square weak_king = board_king_square(board, weak_side);
    const Square weak_pawn = bb_first_square(board_piecetype_bb(board, PAWN));
    Score score = close_bonus(weak_king, strong_king);

    // Check for potential draws with a Pawn on its 7th Rank on either a knight or rook file, and
    // the winning King too far away to avoid the promotion without perpetual checks.
    if (square_rank_relative(weak_pawn, weak_side) != RANK_7
        || square_distance(weak_king, weak_pawn) != 1
        || bb_square_is_set(FILE_B_BB | FILE_D_BB | FILE_E_BB | FILE_G_BB, weak_pawn)) {
        score += QUEEN_EG_SCORE - PAWN_EG_SCORE;
    }

    return board->side_to_move == strong_side ? score : -score;
}

Score eval_kqkr(const Board *board, Color strong_side) {
    const Square strong_king = board_king_square(board, strong_side);
    const Square weak_king = board_king_square(board, color_flip(strong_side));
    const Score score = QUEEN_EG_SCORE - ROOK_EG_SCORE + corner_bonus(weak_king)
        + close_bonus(weak_king, strong_king);
    return board->side_to_move == strong_side ? score : -score;
}

Score eval_knnkp(const Board *board, Color strong_side) {
    // Very complex position. For mating, we need the Pawn sufficiently far away from its promotion
    // square, and the weak King close to an edge.
    const Color weak_side = color_flip(strong_side);
    const Score score = PAWN_EG_SCORE + corner_bonus(board_king_square(board, weak_side))
        - 5 * square_rank_relative(bb_first_square(board_piecetype_bb(board, PAWN)), weak_side);
    return board->side_to_move == strong_side ? score : -score;
}

Score eval_kmpkn(const Board *board, Color strong_side) {
    KingPawnEntry *kpe = king_pawn_probe(board);
    Score score = scorepair_endgame(kpe->value + board->psq_scorepair);
    const Color weak_side = color_flip(strong_side);
    const bool strong_tempo = board->side_to_move == strong_side;
    const Square strong_king = board_king_square(board, strong_side);
    const Square weak_king = board_king_square(board, weak_side);
    const Square strong_pawn = bb_first_square(board_piecetype_bb(board, PAWN));
    const Square weak_knight = bb_first_square(board_piece_bb(board, weak_side, KNIGHT));
    const Bitboard queening_path = forward_file_bb(strong_pawn, strong_side);
    const Bitboard passed_span = passed_pawn_span_bb(strong_pawn, strong_side);

    if (board->side_to_move == BLACK) {
        score = -score;
    }

    // Verifications are simple here: if either the defending King or Knight can efficiently control
    // the promotion path of the Pawn, the position is very drawish.
    if (bb_square_is_set(queening_path, weak_king)) {
        return score / 16;
    }

    if (queening_path & knight_attacks_bb(weak_knight)) {
        return score / 8;
    }

    if (bb_square_is_set(passed_span, weak_king) && !bb_square_is_set(passed_span, strong_king)
        && !strong_tempo) {
        return score / 4;
    }

    return score;
}

Score eval_kmpkb(const Board *board, Color strong_side) {
    KingPawnEntry *kpe = king_pawn_probe(board);
    Score score = scorepair_endgame(kpe->value + board->psq_scorepair);
    const Color weak_side = color_flip(strong_side);
    const bool strong_tempo = board->side_to_move == strong_side;
    const Square strong_king = board_king_square(board, strong_side);
    const Square weak_king = board_king_square(board, weak_side);
    const Square strong_pawn = bb_first_square(board_piecetype_bb(board, PAWN));
    const Square weak_bishop = bb_first_square(board_piece_bb(board, weak_side, BISHOP));
    const Bitboard queening_path = forward_file_bb(strong_pawn, strong_side);
    const Bitboard passed_span = passed_pawn_span_bb(strong_pawn, strong_side);

    if (board->side_to_move == BLACK) {
        score = -score;
    }

    // Verifications are simple here: if either the defending King or Knight can efficiently control
    // the promotion path of the Pawn, the position is very drawish.
    if (bb_square_is_set(queening_path, weak_king)) {
        return score / 16;
    }

    if (queening_path & bishop_attacks_bb(weak_bishop, board_occupancy_bb(board))) {
        return score / 8;
    }

    if (bb_square_is_set(passed_span, weak_king) && !bb_square_is_set(passed_span, strong_king)
        && !strong_tempo) {
        return score / 4;
    }

    return score;
}

Score eval_krpkr(const Board *board, Color strong_side) {
    KingPawnEntry *kpe = king_pawn_probe(board);
    Score score = scorepair_endgame(kpe->value + board->psq_scorepair);
    const Color weak_side = color_flip(strong_side);
    const bool strong_tempo = board->side_to_move == strong_side;
    const Square strong_king = square_relative(board_king_square(board, strong_side), strong_side);
    const Square weak_king = square_relative(board_king_square(board, weak_side), strong_side);
    const Square strong_pawn =
        square_relative(bb_first_square(board_piecetype_bb(board, PAWN)), strong_side);
    const Square weak_rook =
        square_relative(bb_first_square(board_piece_bb(board, weak_side, ROOK)), strong_side);

    if (board->side_to_move == BLACK) {
        score = -score;
    }

    // Check if a Philidor position can be built, or if the back-rank defense can be used.
    if (bb_square_is_set(passed_pawn_span_bb(strong_pawn, WHITE), weak_king)) {
        const Rank sk_rank = square_rank(strong_king);
        const Rank wk_rank = square_rank(weak_king);
        const Rank sp_rank = square_rank(strong_pawn);
        const Rank wr_rank = square_rank(weak_rook);

        if (wk_rank == RANK_6 || wk_rank == RANK_7) {
            if (wr_rank == RANK_5 && sk_rank < RANK_5 && sp_rank < RANK_5) {
                return score / 64;
            }

            if (wr_rank == RANK_1 && sp_rank == RANK_5) {
                return score / 64;
            }
        }

        if (wk_rank >= RANK_7) {
            if (wr_rank == RANK_6 && sk_rank < RANK_6 && sp_rank < RANK_6) {
                return score / 64;
            }

            if (wr_rank <= RANK_2 && sp_rank == RANK_6) {
                return score / 64;
            }
        }

        if (wk_rank == RANK_8 && wr_rank == RANK_8) {
            if (bb_square_is_set(FILE_A_BB | FILE_B_BB | FILE_G_BB | FILE_H_BB, strong_pawn)) {
                return score / 16;
            }

            if (sk_rank + strong_tempo < RANK_6) {
                return score / 16;
            }
        }

        return score / 2;
    }

    return score;
}

Scalefactor scale_kpsk(const Board *board, Color strong_side) {
    const Bitboard strong_pawns = board_piece_bb(board, strong_side, PAWN);

    if (!strong_pawns) {
        return SCALE_DRAW;
    }

    // We only check for draw scenarios with all pawns on a Rook file with the defending King on the
    // promotion path.
    if ((strong_pawns & FILE_A_BB) == strong_pawns || (strong_pawns & FILE_H_BB) == strong_pawns) {
        Square strong_king = board_king_square(board, strong_side);
        Square weak_king = board_king_square(board, color_flip(strong_side));
        Square most_advanced_pawn =
            (strong_side == WHITE) ? bb_last_square(strong_pawns) : bb_first_square(strong_pawns);
        const Color us = strong_side == board->side_to_move ? WHITE : BLACK;
        const bool flip_file = square_file(most_advanced_pawn) >= FILE_E;

        strong_king = normalize_square(strong_side, strong_king, flip_file);
        most_advanced_pawn = normalize_square(strong_side, most_advanced_pawn, flip_file);
        weak_king = normalize_square(strong_side, weak_king, flip_file);

        // If a KPK situation with only the most advanced Pawn results in a draw, then the position
        // is likely a draw as well.
        if (!kpk_bitbase_is_winning(weak_king, strong_king, most_advanced_pawn, us)) {
            return SCALE_DRAW;
        }
    }

    return SCALE_NORMAL;
}

Scalefactor scale_kbpsk(const Board *board, Color strong_side) {
    // If the strong side does not have the bishop, don't try to scale down the endgame.
    if (!board_piece_bb(board, strong_side, BISHOP)) {
        return SCALE_NORMAL;
    }

    const Bitboard strong_pawns = board_piece_bb(board, strong_side, PAWN);

    if (!strong_pawns) {
        return SCALE_DRAW;
    }

    const Color weak_side = color_flip(strong_side);
    const Square strong_bishop =
        square_relative(bb_first_square(board_piecetype_bb(board, BISHOP)), strong_side);
    const Square weak_king = square_relative(board_king_square(board, weak_side), strong_side);
    const Bitboard wrong_file = bb_square_is_set(DSQ_BB, strong_bishop) ? FILE_A_BB : FILE_H_BB;

    // Check for a wrong-colored bishop situation.
    if ((strong_pawns & wrong_file) == strong_pawns) {
        const Square queening_square = bb_square_is_set(DSQ_BB, strong_bishop) ? SQ_A8 : SQ_H8;
        const u8 queening_distance = square_distance(weak_king, queening_square);

        // Check if the weak King is in control of the queening square.
        if (queening_distance < 2) {
            return SCALE_DRAW;
        }

        return SCALE_NORMAL * (i32)(queening_distance - 1) / (i32)queening_distance;
    }

    return SCALE_NORMAL;
}

// Probes the endgame table for the given board. Returns a pointer to the corresponding endgame if
// found, NULL otherwise.
const EndgameEntry *endgame_probe_score(const Board *board) {
    const EndgameEntry *entry = &EndgameTable[board->stack->material_key % ENDGAME_TABLE_SIZE];
    return entry->key == board->stack->material_key && entry->score_fn != NULL ? entry : NULL;
}

// Same as above, except that this one probes for a scaling function rather than a scoring one.
const EndgameEntry *endgame_probe_scale(const Board *board) {
    // Scaling function entries are stored with pawnless material keys.
    Key material_key = board->stack->material_key;

    for (u8 i = 0; i < board_piece_count(board, WHITE_PAWN); ++i) {
        material_key ^= ZobristPsq[WHITE_PAWN][i];
    }

    for (u8 i = 0; i < board_piece_count(board, BLACK_PAWN); ++i) {
        material_key ^= ZobristPsq[BLACK_PAWN][i];
    }

    const EndgameEntry *entry = &EndgameTable[material_key % ENDGAME_TABLE_SIZE];
    return entry->key == material_key && entry->scale_fn != NULL ? entry : NULL;
}
