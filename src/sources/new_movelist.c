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

#include "new_movelist.h"

#include "new_attacks.h"

static ExtendedMove *
    extmove_create_promotions(ExtendedMove *movelist, Square to, Direction direction) {
    (movelist++)->move = create_promotion_move(to - direction, to, QUEEN);
    (movelist++)->move = create_promotion_move(to - direction, to, ROOK);
    (movelist++)->move = create_promotion_move(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion_move(to - direction, to, KNIGHT);

    return movelist;
}

static ExtendedMove *
    extmove_create_underpromotions(ExtendedMove *movelist, Square to, Direction direction) {
    (movelist++)->move = create_promotion_move(to - direction, to, ROOK);
    (movelist++)->move = create_promotion_move(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion_move(to - direction, to, KNIGHT);

    return movelist;
}

static ExtendedMove *extmove_generate_piece_moves(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    Color us,
    Piecetype piecetype,
    Bitboard target
) {
    Bitboard bb = board_piece_bb(board, us, piecetype);
    Bitboard occupancy = board_occupancy_bb(board);

    // Iterate through all pieces of the same type and color, and push the moves
    // for which the arrival square mask intersects the targeted bitboard.
    while (bb) {
        Square from = bb_pop_first_square(&bb);
        Bitboard valid_moves = attacks_bb(piecetype, from, occupancy) & target;

        while (valid_moves) {
            (movelist++)->move = create_move(from, bb_pop_first_square(&valid_moves));
        }
    }

    return movelist;
}

static ExtendedMove *extmove_generate_pawn_noisy(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    Color us,
    Bitboard their_pieces,
    bool in_qsearch
) {
    const Direction pawn_push = pawn_direction(us);
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard pawns_on_last_rank = our_pawns & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    const Bitboard pawns_not_on_last_rank = our_pawns & ~pawns_on_last_rank;
    const Bitboard empty_squares = ~board_occupancy_bb(board);

    if (pawns_on_last_rank) {
        const Bitboard promotion_mask = bb_shift_up_relative(pawns_on_last_rank, us);

        for (Bitboard bb = promotion_mask & empty_squares; bb;) {
            const Square to = bb_pop_first_square(&bb);

            (movelist++)->move = create_promotion_move(to - pawn_push, to, QUEEN);

            if (!in_qsearch) {
                movelist = extmove_create_underpromotions(movelist, to, pawn_push);
            }
        }

        for (Bitboard bb = bb_shift_left(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            (movelist++)->move = create_promotion_move(to - pawn_push - WEST, to, QUEEN);

            if (!in_qsearch) {
                movelist = extmove_create_underpromotions(movelist, to, pawn_push + WEST);
            }
        }

        for (Bitboard bb = bb_shift_right(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            (movelist++)->move = create_promotion_move(to - pawn_push - EAST, to, QUEEN);

            if (!in_qsearch) {
                movelist = extmove_create_underpromotions(movelist, to, pawn_push + EAST);
            }
        }
    }

    const Bitboard capture_mask = bb_shift_up_relative(pawns_not_on_last_rank, us);

    for (Bitboard bb = bb_shift_left(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - WEST, to);
    }

    for (Bitboard bb = bb_shift_right(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - EAST, to);
    }

    if (board->stack->ep_square != SQ_NONE) {
        Bitboard ep_mask =
            pawns_not_on_last_rank & pawn_attacks_bb(board->stack->ep_square, color_flip(us));

        while (ep_mask) {
            (movelist++)->move =
                create_en_passant_move(bb_pop_first_square(&ep_mask), board->stack->ep_square);
        }
    }

    return movelist;
}

static ExtendedMove *extmove_generate_pawn_quiet(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    Color us,
    Bitboard empty_squares
) {
    const Direction pawn_push = pawn_direction(us);
    const Bitboard pawns_not_on_last_rank =
        board_piece_bb(board, us, PAWN) & ~(us == WHITE ? RANK_7_BB : RANK_2_BB);
    Bitboard push_mask = bb_shift_up_relative(pawns_not_on_last_rank, us) & empty_squares;
    Bitboard push2_mask =
        bb_shift_up_relative(push_mask & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & empty_squares;

    while (push_mask) {
        const Square to = bb_pop_first_square(&push_mask);

        (movelist++)->move = create_move(to - pawn_push, to);
    }

    while (push2_mask) {
        const Square to = bb_pop_first_square(&push2_mask);

        (movelist++)->move = create_move(to - pawn_push * 2, to);
    }

    return movelist;
}

static ExtendedMove *extmove_generate_pawn_standard(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    Color us
) {
    const Direction pawn_push = pawn_direction(us);
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard pawns_on_last_rank = our_pawns & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    const Bitboard pawns_not_on_last_rank = our_pawns & ~pawns_on_last_rank;
    const Bitboard empty_squares = ~board_occupancy_bb(board);
    const Bitboard their_pieces = board_color_bb(board, color_flip(us));

    {
        Bitboard push_mask = bb_shift_up_relative(pawns_not_on_last_rank, us) & empty_squares;
        Bitboard push2_mask =
            bb_shift_up_relative(push_mask & (us == WHITE ? RANK_3_BB : RANK_6_BB), us)
            & empty_squares;

        while (push_mask) {
            const Square to = bb_pop_first_square(&push_mask);

            (movelist++)->move = create_move(to - pawn_push, to);
        }

        while (push2_mask) {
            const Square to = bb_pop_first_square(&push2_mask);

            (movelist++)->move = create_move(to - pawn_push * 2, to);
        }
    }

    if (pawns_on_last_rank) {
        const Bitboard promotion_mask = bb_shift_up_relative(pawns_on_last_rank, us);

        for (Bitboard bb = promotion_mask & empty_squares; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push);
        }

        for (Bitboard bb = bb_shift_left(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push + WEST);
        }

        for (Bitboard bb = bb_shift_right(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push + EAST);
        }
    }

    const Bitboard capture_mask = bb_shift_up_relative(pawns_not_on_last_rank, us);

    for (Bitboard bb = bb_shift_left(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - WEST, to);
    }

    for (Bitboard bb = bb_shift_right(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - EAST, to);
    }

    if (board->stack->ep_square != SQ_NONE) {
        Bitboard ep_mask =
            pawns_not_on_last_rank & pawn_attacks_bb(board->stack->ep_square, color_flip(us));

        while (ep_mask) {
            (movelist++)->move =
                create_en_passant_move(bb_pop_first_square(&ep_mask), board->stack->ep_square);
        }
    }

    return movelist;
}

static ExtendedMove *extmove_generate_pawn_incheck(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    Bitboard block_squares,
    Color us
) {
    const Direction pawn_push = pawn_direction(us);
    const Bitboard our_pawns = board_piece_bb(board, us, PAWN);
    const Bitboard pawns_on_last_rank = our_pawns & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    const Bitboard pawns_not_on_last_rank = our_pawns & ~pawns_on_last_rank;
    const Bitboard empty_squares = ~board_occupancy_bb(board);
    const Bitboard their_pieces = board_color_bb(board, color_flip(us)) & block_squares;

    {
        Bitboard push_mask = bb_shift_up_relative(pawns_not_on_last_rank, us) & empty_squares;
        Bitboard push2_mask =
            bb_shift_up_relative(push_mask & (us == WHITE ? RANK_3_BB : RANK_6_BB), us)
            & empty_squares;

        push_mask &= block_squares;
        push2_mask &= block_squares;

        while (push_mask) {
            const Square to = bb_pop_first_square(&push_mask);

            (movelist++)->move = create_move(to - pawn_push, to);
        }

        while (push2_mask) {
            const Square to = bb_pop_first_square(&push2_mask);

            (movelist++)->move = create_move(to - pawn_push * 2, to);
        }
    }

    if (pawns_on_last_rank) {
        const Bitboard promotion_mask = bb_shift_up_relative(pawns_on_last_rank, us);

        for (Bitboard bb = promotion_mask & empty_squares & block_squares; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push);
        }

        for (Bitboard bb = bb_shift_left(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push + WEST);
        }

        for (Bitboard bb = bb_shift_right(promotion_mask) & their_pieces; bb;) {
            const Square to = bb_pop_first_square(&bb);

            movelist = extmove_create_promotions(movelist, to, pawn_push + EAST);
        }
    }

    const Bitboard capture_mask = bb_shift_up_relative(pawns_not_on_last_rank, us);

    for (Bitboard bb = bb_shift_left(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - WEST, to);
    }

    for (Bitboard bb = bb_shift_right(capture_mask) & their_pieces; bb;) {
        const Square to = bb_pop_first_square(&bb);

        (movelist++)->move = create_move(to - pawn_push - EAST, to);
    }

    if (board->stack->ep_square != SQ_NONE
        && bb_square_is_set(block_squares, board->stack->ep_square - pawn_push)) {
        Bitboard ep_mask =
            pawns_not_on_last_rank & pawn_attacks_bb(board->stack->ep_square, color_flip(us));

        while (ep_mask) {
            (movelist++)->move =
                create_en_passant_move(bb_pop_first_square(&ep_mask), board->stack->ep_square);
        }
    }

    return movelist;
}

ExtendedMove *extmove_generate_legal(ExtendedMove *restrict movelist, const Board *restrict board) {
    const Color us = board->side_to_move;
    const Bitboard pinned = board->stack->king_blockers[us] & board_color_bb(board, us);
    const Square king_square = board_king_square(board, us);
    ExtendedMove *it = movelist;

    movelist = board->stack->checkers ? extmove_generate_incheck(movelist, board)
                                      : extmove_generate_standard(movelist, board);

    while (it != movelist) {
        if ((pinned || move_from(it->move) == king_square || move_type(it->move) == EN_PASSANT)
            && !board_move_is_legal(board, it->move)) {
            it->move = (--movelist)->move;
        } else {
            ++it;
        }
    }

    return movelist;
}

ExtendedMove *
    extmove_generate_standard(ExtendedMove *restrict movelist, const Board *restrict board) {
    const Color us = board->side_to_move;
    const Bitboard target_squares = ~board_color_bb(board, us);
    const Square our_king = board_king_square(board, us);

    movelist = extmove_generate_pawn_standard(movelist, board, us);

    for (Piecetype piecetype = KNIGHT; piecetype <= KING; ++piecetype) {
        movelist = extmove_generate_piece_moves(movelist, board, us, piecetype, target_squares);
    }

    CastlingRights kingside = us == WHITE ? WHITE_OO : BLACK_OO;
    CastlingRights queenside = us == WHITE ? WHITE_OOO : BLACK_OOO;

    if ((board->stack->castlings & kingside) && !board_castling_is_blocked(board, kingside)) {
        (movelist++)->move = create_castling_move(our_king, board->castling_rook_square[kingside]);
    }

    if ((board->stack->castlings & queenside) && !board_castling_is_blocked(board, queenside)) {
        (movelist++)->move = create_castling_move(our_king, board->castling_rook_square[queenside]);
    }

    return movelist;
}

ExtendedMove *
    extmove_generate_incheck(ExtendedMove *restrict movelist, const Board *restrict board) {
    const Color us = board->side_to_move;
    const Square king_square = board_king_square(board, us);
    Bitboard slider_attacks = 0;

    for (Bitboard sliders = board->stack->checkers & ~board_piecetypes_bb(board, KNIGHT, PAWN);
         sliders;) {
        const Square check_square = bb_pop_first_square(&sliders);
        slider_attacks |= line_bb(check_square, king_square) ^ square_bb(check_square);
    }

    for (Bitboard bb = king_attacks_bb(king_square) & ~board_color_bb(board, us) & ~slider_attacks;
         bb;) {
        (movelist++)->move = create_move(king_square, bb_pop_first_square(&bb));
    }

    // If in check in multiple times, we know only King moves can be legal.
    if (bb_more_than_one(board->stack->checkers)) {
        return movelist;
    }

    const Square check_square = bb_first_square(board->stack->checkers);
    const Bitboard target = between_squares_bb(check_square, king_square) | square_bb(check_square);

    movelist = extmove_generate_pawn_incheck(movelist, board, target, us);

    for (Piecetype piecetype = KNIGHT; piecetype <= QUEEN; ++piecetype) {
        movelist = extmove_generate_piece_moves(movelist, board, us, piecetype, target);
    }

    return movelist;
}

ExtendedMove *extmove_generate_noisy(
    ExtendedMove *restrict movelist,
    const Board *restrict board,
    bool in_qsearch
) {
    const Color us = board->side_to_move;
    const Bitboard their_pieces = board_color_bb(board, color_flip(us));

    movelist = extmove_generate_pawn_noisy(movelist, board, us, their_pieces, in_qsearch);

    for (Piecetype piecetype = KNIGHT; piecetype <= KING; ++piecetype) {
        movelist = extmove_generate_piece_moves(movelist, board, us, piecetype, their_pieces);
    }

    return movelist;
}

ExtendedMove *extmove_generate_quiet(ExtendedMove *restrict movelist, const Board *restrict board) {
    const Color us = board->side_to_move;
    const Bitboard empty_squares = ~board_occupancy_bb(board);
    const Square our_king = board_king_square(board, us);

    movelist = extmove_generate_pawn_quiet(movelist, board, us, empty_squares);

    for (Piecetype piecetype = KNIGHT; piecetype <= KING; ++piecetype) {
        movelist = extmove_generate_piece_moves(movelist, board, us, piecetype, empty_squares);
    }

    CastlingRights kingside = us == WHITE ? WHITE_OO : BLACK_OO;
    CastlingRights queenside = us == WHITE ? WHITE_OOO : BLACK_OOO;

    if ((board->stack->castlings & kingside) && !board_castling_is_blocked(board, kingside)) {
        (movelist++)->move = create_castling_move(our_king, board->castling_rook_square[kingside]);
    }

    if ((board->stack->castlings & queenside) && !board_castling_is_blocked(board, queenside)) {
        (movelist++)->move = create_castling_move(our_king, board->castling_rook_square[queenside]);
    }

    return movelist;
}

void extmove_pick_best(ExtendedMove *begin, ExtendedMove *end) {
    ExtendedMove tmp;
    ExtendedMove *best = begin;

    for (ExtendedMove *it = begin + 1; it != end; ++it) {
        if (it->score > best->score) {
            best = it;
        }
    }

    tmp = *best;
    *best = *begin;
    *begin = tmp;
}

bool movelist_contains(const Movelist *movelist, Move move) {
    for (const ExtendedMove *it = movelist_begin(movelist); it != movelist_end(movelist); ++it) {
        if (it->move == move) {
            return true;
        }
    }

    return false;
}
