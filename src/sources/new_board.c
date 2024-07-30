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

#include "new_board.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "new_attacks.h"
#include "new_bitboard.h"
#include "new_chess_types.h"
#include "new_core.h"
#include "new_hashkey.h"
#include "new_movelist.h"
#include "new_psq_table.h"
#include "new_wmalloc.h"

const StringView PieceIndexes = STATIC_STRVIEW(" PNBRQK  pnbrqk ");
const StringView PromotionIndexes = STATIC_STRVIEW("  nbrq  ");
const StringView StartposStr =
    STATIC_STRVIEW("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

Key CyclicKeys[0x2000];
Move CyclicMoves[0x2000];

static Bitboard board_attackers_to(const Board *board, Square square);
static Bitboard board_slider_blockers(
    const Board *restrict board,
    Bitboard sliders,
    Square square,
    Bitboard *restrict pinners
);

INLINED u16 cyclic_index_lo(Key key) {
    return key & 0x1FFFu;
}

INLINED u16 cyclic_index_hi(Key key) {
    return (key >> 13) & 0x1FFFu;
}

static void cyclic_init_move(Piece piece, Square from, Square to) {
    Move move = create_move(from, to);
    Key key = ZobristPsq[piece][from] ^ ZobristPsq[piece][to] ^ ZobristSideToMove;
    u16 index = cyclic_index_lo(key);

    // Swap the current move/key pair with the table contents until we find an
    // empty slot.
    while (move != NO_MOVE) {
        Key tmp_key = CyclicKeys[index];
        CyclicKeys[index] = key;
        key = tmp_key;

        Move tmp_move = CyclicMoves[index];
        CyclicMoves[index] = move;
        move = tmp_move;

        // Trick: change the section of the key for indexing by xor-ing the
        // index value with the low and high key indexes:
        // - if index == hi, index ^ lo ^ hi == lo
        // - if index == lo, index ^ lo ^ hi == hi
        index ^= cyclic_index_lo(key) ^ cyclic_index_hi(key);
    }
}

void cyclic_init(void) {
    // Map all reversible move Zobrist keys to their corresponding move
    for (Piecetype piecetype = KNIGHT; piecetype <= KING; ++piecetype) {
        for (Color color = WHITE; color <= BLACK; ++color) {
            for (Square from = SQ_A1; from <= SQ_H8; ++from) {
                for (Square to = from + 1; to <= SQ_H8; ++to) {
                    if (bb_square_is_set(attacks_bb(piecetype, from, 0), to)) {
                        cyclic_init_move(create_piece(color, piecetype), from, to);
                    }
                }
            }
        }
    }
}

static void boardstack_set_check_info(Boardstack *restrict stack, const Board *restrict board) {
    const Square their_king_square = board_king_square(board, color_flip(board->side_to_move));

    // Store the list of pieces that are currently pinned to their King.
    stack->king_blockers[WHITE] = board_slider_blockers(
        board,
        board_color_bb(board, BLACK),
        board_king_square(board, WHITE),
        &stack->pinners[BLACK]
    );
    stack->king_blockers[BLACK] = board_slider_blockers(
        board,
        board_color_bb(board, WHITE),
        board_king_square(board, BLACK),
        &stack->pinners[WHITE]
    );

    // Compute the list of squares from which the side to move can give check
    // to the opponent's King.
    stack->check_squares[PAWN] =
        pawn_attacks_bb(their_king_square, color_flip(board->side_to_move));
    stack->check_squares[KNIGHT] = knight_attacks_bb(their_king_square);
    stack->check_squares[BISHOP] = bishop_attacks_bb(their_king_square, board_occupancy_bb(board));
    stack->check_squares[ROOK] = rook_attacks_bb(their_king_square, board_occupancy_bb(board));
    stack->check_squares[QUEEN] = stack->check_squares[BISHOP] | stack->check_squares[ROOK];
    stack->check_squares[KING] = 0;
}

void boardstack_init(Boardstack *restrict stack, const Board *restrict board) {
    stack->board_key = stack->king_pawn_key = stack->material_key = 0;
    stack->material[WHITE] = stack->material[BLACK] = 0;
    stack->checkers = board_attackers_to(board, board_king_square(board, board->side_to_move))
        & board_color_bb(board, color_flip(board->side_to_move));

    boardstack_set_check_info(stack, board);

    // Compute the board key, pawn key, and material distribution values from
    // the occupancy bitboards.
    for (Bitboard bb = board_occupancy_bb(board); bb;) {
        const Square square = bb_pop_first_square(&bb);
        const Piece piece = board_piece_on(board, square);

        stack->board_key ^= ZobristPsq[piece][square];

        if (piece_type(piece) == PAWN || piece_type(piece) == KING) {
            stack->king_pawn_key ^= ZobristPsq[piece][square];
        } else {
            stack->material[piece_color(piece)] += PieceScores[MIDGAME][piece];
        }
    }

    if (stack->ep_square != SQ_NONE) {
        stack->board_key ^= ZobristEnPassant[square_file(stack->ep_square)];
    }

    if (board->side_to_move == BLACK) {
        stack->board_key ^= ZobristSideToMove;
    }

    // Compute the material key used for indexing specialized endgames.
    for (Color color = WHITE; color <= BLACK; ++color) {
        for (Piecetype piecetype = PAWN; piecetype <= KING; ++piecetype) {
            const Piece piece = create_piece(color, piecetype);

            for (u8 i = 0; i < board->piece_count[piece]; ++i) {
                stack->material_key ^= ZobristPsq[piece][i];
            }
        }
    }

    stack->board_key ^= ZobristCastling[stack->castlings];
}

Boardstack *boardstack_clone(const Boardstack *stack) {
    if (stack == NULL) {
        return NULL;
    }

    Boardstack *const new_stack = wrap_malloc(sizeof(Boardstack));
    Boardstack *istack = new_stack;

    *istack = *stack;

    while (stack->previous != NULL) {
        istack->previous = wrap_malloc(sizeof(Boardstack));
        istack = istack->previous;
        stack = stack->previous;
        *istack = *stack;
    }

    istack->previous = NULL;
    return new_stack;
}

void boardstack_destroy(Boardstack *stack) {
    while (stack) {
        Boardstack *const prev = stack->previous;

        free(stack);
        stack = prev;
    }
}

// Helper function for putting pieces on the board
static void board_put_piece(Board *board, Piece piece, Square square) {
    assert(piece_is_valid(piece));
    assert(square_is_valid(square));

    Bitboard sqbb = square_bb(square);

    board->mailbox[square] = piece;
    board->piecetype_bb[ALL_PIECES] |= sqbb;
    board->piecetype_bb[piece_type(piece)] |= sqbb;
    board->color_bb[piece_color(piece)] |= sqbb;
    ++board->piece_count[piece];
    ++board->piece_count[create_piece(piece_color(piece), ALL_PIECES)];
    board->psq_scorepair += psq_table(piece, square);
}

// Helper function for moving pieces on the board
static void board_move_piece(Board *board, Square from, Square to) {
    assert(square_is_valid(from));
    assert(square_is_valid(to));
    assert(from != to);

    Piece piece = board_piece_on(board, from);
    Bitboard move_bb = square_bb(from) | square_bb(to);

    board->mailbox[from] = NO_PIECE;
    board->mailbox[to] = piece;
    board->piecetype_bb[ALL_PIECES] ^= move_bb;
    board->piecetype_bb[piece_type(piece)] ^= move_bb;
    board->color_bb[piece_color(piece)] ^= move_bb;
    board->psq_scorepair += psq_table(piece, to) - psq_table(piece, from);
}

// Helper function for taking pieces off the board
static void board_remove_piece(Board *board, Square square) {
    assert(square_is_valid(square));
    assert(!board_square_is_empty(board, square));

    Piece piece = board_piece_on(board, square);
    Bitboard notsq_bb = ~square_bb(square);

    board->mailbox[square] = NO_PIECE;
    board->piecetype_bb[ALL_PIECES] &= notsq_bb;
    board->piecetype_bb[piece_type(piece)] &= notsq_bb;
    board->color_bb[piece_color(piece)] &= notsq_bb;
    --board->piece_count[piece];
    --board->piece_count[create_piece(piece_color(piece), ALL_PIECES)];
    board->psq_scorepair -= psq_table(piece, square);
}

static Bitboard board_attackers_list(const Board *board, Square square, Bitboard occupancy) {
    return (
        (pawn_attacks_bb(square, BLACK) & board_piece_bb(board, WHITE, PAWN))
        | (pawn_attacks_bb(square, WHITE) & board_piece_bb(board, BLACK, PAWN))
        | (knight_attacks_bb(square) & board_piecetype_bb(board, KNIGHT))
        | (bishop_attacks_bb(square, occupancy) & board_piecetypes_bb(board, BISHOP, QUEEN))
        | (rook_attacks_bb(square, occupancy) & board_piecetypes_bb(board, ROOK, QUEEN))
        | (king_attacks_bb(square) & board_piecetype_bb(board, KING))
    );
}

// Returns a bitboard of all friendly/enemy pieces attacking the given square.
static Bitboard board_attackers_to(const Board *board, Square square) {
    assert(square_is_valid(square));
    return board_attackers_list(board, square, board_occupancy_bb(board));
}

// Returns the bitboard of all pieces preventing attacks on the given square.
static Bitboard board_slider_blockers(
    const Board *restrict board,
    Bitboard sliders,
    Square square,
    Bitboard *restrict pinners
) {
    Bitboard blockers = *pinners = 0;
    Bitboard snipers =
        ((rook_raw_attacks_bb(square) & board_piecetypes_bb(board, ROOK, QUEEN))
         | (bishop_raw_attacks_bb(square) & board_piecetypes_bb(board, BISHOP, QUEEN)))
        & sliders;
    const Bitboard occupancy = board_occupancy_bb(board) ^ snipers;

    while (snipers) {
        const Square sniper_square = bb_pop_first_square(&snipers);
        const Bitboard between = between_squares_bb(square, sniper_square) & occupancy;

        // There are no pins if there are two or more pieces between the slider
        // piece and the target square.
        if (between && !bb_more_than_one(between)) {
            blockers |= between;
            if (between & board_color_bb(board, piece_color(board_piece_on(board, square)))) {
                bb_set_square(pinners, sniper_square);
            }
        }
    }

    return blockers;
}

static bool board_has_invalid_material(const Board *board, Color color) {
    const u8 knights = board_piece_count(board, create_piece(color, KNIGHT));
    const u8 bishops = board_piece_count(board, create_piece(color, BISHOP));
    const u8 rooks = board_piece_count(board, create_piece(color, ROOK));
    const u8 queens = board_piece_count(board, create_piece(color, QUEEN));
    u8 pawns = board_piece_count(board, create_piece(color, PAWN));

    // Compute the number of pieces that are guaranteed to be promoted Pawns.
    const u8 pknights = u8_max(2, knights) - 2;
    const u8 pbishops = u8_max(2, bishops) - 2;
    const u8 prooks = u8_max(2, rooks) - 2;
    const u8 pqueens = u8_max(2, queens) - 2;
    const u8 promoted = pknights + pbishops + prooks + pqueens;

    pawns += promoted;
    return (pawns > 8) || (knights + pawns - pknights > 10) || (bishops + pawns - pbishops > 10)
        || (rooks + pawns - prooks > 10) || (queens + pawns - pqueens > 9);
}

static bool board_parse_fen_pieces(Board *board, StringView piece_field) {
    usize piece_index;
    File file = FILE_A;
    Rank rank = RANK_8;

    for (usize i = 0; i < piece_field.size; ++i) {
        u8 c = piece_field.data[i];

        if (c >= '1' && c <= '8') {
            file += c - '0';

            if (file > FILE_NB) {
                // info_debug()
                return false;
            }
        } else if (c == '/') {
            if (file != FILE_NB) {
                // info_debug()
                return false;
            }

            if (rank == RANK_1) {
                // info_debug()
                return false;
            }

            --rank;
            file = FILE_A;
        } else if (piece_index = strview_find(PieceIndexes, c), piece_index != NPOS) {
            if (!file_is_valid(file)) {
                // info_debug()
                return false;
            }

            board_put_piece(board, (Piece)piece_index, create_square(file, rank));
            ++file;
        } else {
            // info_debug()
            return false;
        }
    }

    if (rank != RANK_1) {
        // info_debug()
        return false;
    }

    if (file != FILE_NB) {
        // info_debug()
        return false;
    }

    if (board_piece_count(board, WHITE_KING) != 1 || board_piece_count(board, BLACK_KING) != 1) {
        // info_debug()
        return false;
    }

    if (square_distance(board_king_square(board, WHITE), board_king_square(board, BLACK)) == 1) {
        // info_debug()
        return false;
    }

    if (board_piecetype_bb(board, PAWN) & (RANK_1_BB | RANK_8_BB)) {
        // info_debug()
        return false;
    }

    if (board_has_invalid_material(board, WHITE) || board_has_invalid_material(board, BLACK)) {
        // info_debug()
        return false;
    }

    return true;
}

static bool board_parse_stm(Board *board, StringView stm_field) {
    if (stm_field.size > 1) {
        // info_debug()
        return false;
    }

    if (stm_field.size == 0 || stm_field.data[0] == 'w') {
        board->side_to_move = WHITE;
    } else if (stm_field.data[0] == 'b') {
        board->side_to_move = BLACK;
    } else {
        // info_debug()
        return false;
    }

    return true;
}

static bool board_set_castling(Board *board, Color color, Square rook_square) {
    const Square king_square = board_king_square(board, color);
    const CastlingRights castling =
        relative_cr(color) & (king_square < rook_square ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

    if (square_rank_relative(king_square, color) != RANK_1) {
        // info_debug()
        return false;
    }

    if (square_file(king_square) != FILE_E
        || (square_file(rook_square) != FILE_A && square_file(rook_square) != FILE_H)) {
        board->chess960 = true;
    }

    board->stack->castlings |= castling;
    board->castling_mask[king_square] |= castling;
    board->castling_mask[rook_square] |= castling;
    board->castling_rook_square[castling] = rook_square;

    const Square king_after =
        square_relative((castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1), color);
    const Square rook_after =
        square_relative((castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1), color);

    board->castling_path[castling] =
        (between_squares_bb(rook_square, rook_after) | between_squares_bb(king_square, king_after)
         | square_bb(rook_after) | square_bb(king_after))
        & ~(square_bb(king_square) | square_bb(rook_square));

    return true;
}

static bool board_parse_castling(Board *board, StringView castling_field) {
    for (usize i = 0; i < castling_field.size; ++i) {
        if (castling_field.data[i] == '-') {
            if (castling_field.size > 1) {
                // info_debug()
                return false;
            }

            break;
        }

        const Color side = islower(castling_field.data[i]) ? BLACK : WHITE;
        const Piece rook = create_piece(side, ROOK);
        const u8 castling_char = toupper(castling_field.data[i]);
        Square rook_square;

        if (castling_char == 'K') {
            for (rook_square = square_relative(SQ_H1, side); square_file(rook_square) != FILE_A;
                 --rook_square) {
                if (board_piece_on(board, rook_square) == rook) {
                    break;
                }
            }
        } else if (castling_char == 'Q') {
            for (rook_square = square_relative(SQ_A1, side); square_file(rook_square) != FILE_H;
                 ++rook_square) {
                if (board_piece_on(board, rook_square) == rook) {
                    break;
                }
            }
        } else if (castling_char >= 'A' && castling_char <= 'H') {
            rook_square = create_square(castling_char - 'A', rank_relative(RANK_1, side));
        } else {
            // info_debug()
            return false;
        }

        if (!board_set_castling(board, side, rook_square)) {
            return false;
        }
    }

    return true;
}

static bool board_parse_en_passant(Board *board, StringView ep_field) {
    board->stack->ep_square = SQ_NONE;

    if (ep_field.size == 1 && ep_field.data[0] != '-') {
        // info_debug()
        return false;
    } else if (ep_field.size > 2) {
        // info_debug()
        return false;
    } else if (ep_field.size <= 1) {
        return true;
    }

    const u8 file_char = ep_field.data[0];
    const u8 rank_char = ep_field.data[1];

    if (file_char < 'a' || file_char > 'h'
        || rank_char != (board->side_to_move == WHITE ? '6' : '3')) {
        // info_debug()
        return false;
    }

    const Square ep_square = create_square(file_char - 'a', rank_char - '1');
    const Color us = board->side_to_move;
    const Color them = color_flip(us);

    if (!bb_square_is_set(board_piece_bb(board, them, PAWN), ep_square + pawn_direction(them))) {
        // info_debug()
        return false;
    }

    if (pawn_attacks_bb(ep_square, them) & board_piece_bb(board, us, PAWN)) {
        board->stack->ep_square = ep_square;
    }

    return true;
}

static bool board_parse_rule50(Board *board, StringView rule50_field) {
    u64 value;

    if (rule50_field.size == 0) {
        board->stack->rule50 = 0;
        return true;
    }

    if (!strview_parse_u64(rule50_field, &value)) {
        // info_debug()
        return false;
    }

    if (value > 150) {
        // info_debug()
        return false;
    }

    board->stack->rule50 = value;
    return true;
}

static bool board_parse_movenumber(Board *board, StringView movenumber_field) {
    u64 value;

    if (movenumber_field.size == 0) {
        board->ply = 0;
        return true;
    }

    if (!strview_parse_u64(movenumber_field, &value)) {
        // info_debug()
        return false;
    }

    if (value > 8848) {
        // info_debug()
        return false;
    }

    board->ply = (u64_max(value, 1) - 1) * 2 + (board->side_to_move == BLACK);
    return true;
}

bool board_try_init(Board *board, StringView fen, bool is_chess960, Boardstack *stack) {
    memset(board, 0, sizeof(Board));
    memset(stack, 0, sizeof(Boardstack));
    board->stack = stack;

    if (!board_parse_fen_pieces(board, strview_next_word(&fen))) {
        return false;
    }

    if (!board_parse_stm(board, strview_next_word(&fen))) {
        return false;
    }

    if (board_attackers_to(board, board_king_square(board, color_flip(board->side_to_move)))
        & board_color_bb(board, board->side_to_move)) {
        // info_debug()
        return false;
    }

    if (!board_parse_castling(board, strview_next_word(&fen))) {
        return false;
    }

    if (!is_chess960 && board->chess960) {
        // info_debug()
    }

    board->chess960 = board->chess960 || is_chess960;

    if (!board_parse_en_passant(board, strview_next_word(&fen))) {
        return false;
    }

    if (!board_parse_rule50(board, strview_next_word(&fen))) {
        return false;
    }

    if (!board_parse_movenumber(board, strview_next_word(&fen))) {
        return false;
    }

    boardstack_init(stack, board);

    if (bb_popcount(board->stack->checkers) > 2) {
        // info_debug()
        return false;
    }

    return true;
}

void board_clone(Board *restrict board, const Board *restrict other) {
    *board = *other;
    board->has_worker = false;
    board->stack = boardstack_clone(other->stack);
}

static void barray_append_uint(u8 *buffer, usize *size, u64 value) {
    usize n = 0;

    do {
        buffer[*size + n] = (value % 10) + '0';
        value /= 10;
        ++n;
    } while (value != 0);

    for (usize i = 0; i < n / 2; ++i) {
        u8 tmp = buffer[*size + i];

        buffer[*size + i] = buffer[*size + n - i - 1];
        buffer[*size + n - i - 1] = tmp;
    }

    *size += n;
}

StringView board_get_fen(const Board *board) {
    static u8 fen_buffer[128];
    usize size = 0;

    for (Rank rank = RANK_8; rank_is_valid(rank); --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) {
            u8 empty_count = 0;

            while (file <= FILE_H && board_square_is_empty(board, create_square(file, rank))) {
                ++empty_count;
                ++file;
            }

            if (empty_count != 0) {
                fen_buffer[size++] = empty_count + '0';
            }

            if (file <= FILE_H) {
                fen_buffer[size++] =
                    PieceIndexes.data[board_piece_on(board, create_square(file, rank))];
            }
        }

        if (rank != RANK_1) {
            fen_buffer[size++] = '/';
        }
    }

    fen_buffer[size++] = ' ';
    fen_buffer[size++] = (board->side_to_move == WHITE) ? 'w' : 'b';
    fen_buffer[size++] = ' ';

    if (board->stack->castlings & WHITE_OO) {
        fen_buffer[size++] =
            board->chess960 ? 'A' + square_file(board->castling_rook_square[WHITE_OO]) : 'K';
    }

    if (board->stack->castlings & WHITE_OOO) {
        fen_buffer[size++] =
            board->chess960 ? 'A' + square_file(board->castling_rook_square[WHITE_OOO]) : 'Q';
    }

    if (board->stack->castlings & BLACK_OO) {
        fen_buffer[size++] =
            board->chess960 ? 'a' + square_file(board->castling_rook_square[BLACK_OO]) : 'k';
    }

    if (board->stack->castlings & BLACK_OOO) {
        fen_buffer[size++] =
            board->chess960 ? 'a' + square_file(board->castling_rook_square[BLACK_OOO]) : 'q';
    }

    if (!board->stack->castlings) {
        fen_buffer[size++] = '-';
    }

    fen_buffer[size++] = ' ';

    if (board->stack->ep_square == SQ_NONE) {
        fen_buffer[size++] = '-';
    }

    else {
        fen_buffer[size++] = 'a' + square_file(board->stack->ep_square);
        fen_buffer[size++] = '1' + square_rank(board->stack->ep_square);
    }

    fen_buffer[size++] = ' ';
    barray_append_uint(fen_buffer, &size, board->stack->rule50);
    fen_buffer[size++] = ' ';
    barray_append_uint(fen_buffer, &size, 1 + (board->ply - (board->side_to_move == BLACK)) / 2);
    return strview_from_raw_data(fen_buffer, size);
}

bool board_move_is_pseudolegal(const Board *board, Move move) {
    const Color us = board->side_to_move;
    const Color them = color_flip(board->side_to_move);
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece piece = board_piece_on(board, from);

    // We perform a slower, but simpler check for "non-standard" moves.
    if (move_type(move) != NORMAL_MOVE) {
        Movelist movelist;

        movelist_generate_pseudo(&movelist, board);
        return movelist_contains(&movelist, move);
    }

    // The move is normal, so the promotion type bits cannot be set.
    if (move_promotion_type(move) != KNIGHT) {
        return false;
    }

    // Check if there is a piece that belongs to us on the 'from' square.
    if (piece == NO_PIECE || piece_color(piece) != us) {
        return false;
    }

    // Check if the arrival square doesn't contain one of our pieces. (Note that even though
    // castling is encoded as 'King takes Rook', it is handled in the "non-standard" scope above.)
    if (bb_square_is_set(board_color_bb(board, us), to)) {
        return false;
    }

    if (piece_type(piece) == PAWN) {
        // The Pawn cannot arrive on a promotion square, since the move is not flagged as a
        // promotion.
        if (bb_square_is_set(RANK_1_BB | RANK_8_BB, to)) {
            return false;
        }

        // Check if the Pawn move is a valid capture, push, or double push.
        if (!bb_square_is_set(pawn_attacks_bb(from, us) & board_color_bb(board, them), to)
            && !(from + pawn_direction(us) == to && board_square_is_empty(board, to))
            && !(
                from + 2 * pawn_direction(us) == to && square_rank_relative(from, us) == RANK_2
                && board_square_is_empty(board, to)
                && board_square_is_empty(board, to - pawn_direction(us))
            )) {
            return false;
        }
    }
    // Check if the piece can reach the arrival square from its position.
    else if (!bb_square_is_set(
                 attacks_bb(piece_type(piece), from, board_occupancy_bb(board)),
                 to
             )) {
        return false;
    }

    if (board->stack->checkers) {
        if (piece_type(piece) != KING) {
            // If the side to move is in double check, only a King move can be pseudo-legal.
            if (bb_more_than_one(board->stack->checkers)) {
                return false;
            }

            // We must either capture the checking piece, or block the attack if it's a slider
            // piece.
            const Bitboard target = between_squares_bb(
                                        bb_first_square(board->stack->checkers),
                                        board_king_square(board, us)
                                    )
                | board->stack->checkers;

            if (!bb_square_is_set(target, to)) {
                return false;
            }
        }
        // Check if the King is still under the fire of the opponent's pieces after moving.
        else if (!!(board_attackers_list(board, to, board_occupancy_bb(board) ^ square_bb(from))
                    & board_color_bb(board, them))) {
            return false;
        }
    }

    return true;
}

bool board_move_is_legal(const Board *board, Move move) {
    const Color us = board->side_to_move;
    const Color them = color_flip(board->side_to_move);
    const Square from = move_from(move);
    Square to = move_to(move);

    // Special handling for en-passant captures.
    if (move_type(move) == EN_PASSANT) {
        // Check for any discovered check on our King with the capture.
        const Square king_square = board_king_square(board, us);
        const Square capture_square = to - pawn_direction(us);
        const Bitboard occupancy_after =
            (board_occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
            | square_bb(to);

        return !(bishop_attacks_bb(king_square, occupancy_after)
                 & board_pieces_bb(board, them, BISHOP, QUEEN))
            && !(
                rook_attacks_bb(king_square, occupancy_after)
                & board_pieces_bb(board, them, ROOK, QUEEN)
            );
    }

    // Special handling for castling moves.
    if (move_type(move) == CASTLING) {
        const Direction direction = to > from ? WEST : EAST;

        to = square_relative(to > from ? SQ_G1 : SQ_C1, us);

        // Check for any opponent's piece attack along the King path.
        for (Square square = to; square != from; square += direction) {
            if (board_attackers_to(board, square) & board_color_bb(board, them)) {
                return false;
            }
        }

        // For Chess960, check for a discovered check after removing the castling Rook.
        return !board->chess960
            || !(
                rook_attacks_bb(to, board_occupancy_bb(board) ^ square_bb(move_to(move)))
                & board_pieces_bb(board, them, ROOK, QUEEN)
            );
    }

    // If the King is moving, check for any opponent's piece attack on the arrival square.
    if (piece_type(board_piece_on(board, from)) == KING) {
        return !(board_attackers_to(board, to) & board_color_bb(board, them));
    }

    // If the moving piece is pinned, check if the move generates a discovered check.
    return !bb_square_is_set(board->stack->king_blockers[us], from)
        || squares_are_aligned(from, to, board_king_square(board, us));
}

bool board_move_gives_check(const Board *board, Move move) {
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Color us = board->side_to_move;
    const Color them = color_flip(board->side_to_move);
    const Square their_king = board_king_square(board, them);
    const Movetype movetype = move_type(move);

    // Test if the move is a direct check to the King.
    if (bb_square_is_set(
            board->stack->check_squares[piece_type(board_piece_on(board, from))],
            to
        )) {
        return true;
    }

    // Test if the move is a discovered check.
    if (bb_square_is_set(board->stack->king_blockers[them], from)
        && !squares_are_aligned(from, to, their_king)) {
        return true;
    }

    // In the case of a promotion, test if the promoted piece gives a direct check to the King from
    // the arrival square.
    if (movetype == PROMOTION) {
        return bb_square_is_set(
            attacks_bb(move_promotion_type(move), to, board_occupancy_bb(board) ^ square_bb(from)),
            their_king
        );
    }

    // In the case of en passant captures, test if either the captured Pawn or the moving Pawn are
    // uncovering an attack on the King.
    if (movetype == EN_PASSANT) {
        const Square capture_square = create_square(square_file(to), square_rank(from));
        const Bitboard occupancy_after =
            (board_occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
            | square_bb(to);

        return (bishop_attacks_bb(their_king, occupancy_after)
                & board_pieces_bb(board, us, BISHOP, QUEEN))
            || (rook_attacks_bb(their_king, occupancy_after)
                & board_pieces_bb(board, us, ROOK, QUEEN));
    }

    // In the case of a castling move, test if the Rook gives a direct check from its arrival
    // square.
    if (movetype == CASTLING) {
        const Square king_from = from;
        const Square rook_from = to;
        const Square king_to = square_relative(rook_from > king_from ? SQ_G1 : SQ_C1, us);
        const Square rook_to = square_relative(rook_from > king_from ? SQ_F1 : SQ_D1, us);

        // Early verification: if the Rook doesn't see the King on an empty board, don't do the
        // expensive check afterwards.
        if (!bb_square_is_set(rook_raw_attacks_bb(rook_to), their_king)) {
            return false;
        }

        // Do the verification with the updated occupancy.
        return bb_square_is_set(
            rook_attacks_bb(
                rook_to,
                (board_occupancy_bb(board) ^ square_bb(king_from) ^ square_bb(rook_from))
                    | square_bb(king_to) | square_bb(rook_to)
            ),
            their_king
        );
    }

    return false;
}

static void board_do_castling(
    Board *restrict board,
    Color us,
    Square king_from,
    Square *restrict king_to,
    Square *restrict rook_from,
    Square *restrict rook_to
) {
    // Get the castling side based on whether the Rook is on the left or the right side of the
    // King.
    const bool kingside = *king_to > king_from;

    // Since we encode castling moves as "King takes Rook", the Rook initial position corresponds
    // to the arrival square of the move. We then compute the real arrival squares of the King and
    // Rook based on the castling side.
    *rook_from = *king_to;
    *rook_to = square_relative(kingside ? SQ_F1 : SQ_D1, us);
    *king_to = square_relative(kingside ? SQ_G1 : SQ_C1, us);

    // Update the King and Rook positions on the board.
    board_remove_piece(board, king_from);
    board_remove_piece(board, *rook_from);
    board_put_piece(board, create_piece(us, KING), *king_to);
    board_put_piece(board, create_piece(us, ROOK), *rook_to);
}

void board_do_move_gc(
    Board *restrict board,
    Move move,
    Boardstack *restrict new_stack,
    bool gives_check
) {
    const Color us = board->side_to_move;
    const Color them = color_flip(board->side_to_move);
    const Square from = move_from(move);
    Square to = move_to(move);
    const Piece piece = board_piece_on(board, from);
    Piece captured_piece =
        move_type(move) == EN_PASSANT ? create_piece(them, PAWN) : board_piece_on(board, to);
    Key key = board->stack->board_key ^ ZobristSideToMove;

    assert(piece_type(captured_piece) != KING);

    // Copy the state variables that will need to be updated incrementally. Don't copy things like
    // the checking squares, since they need to be computed from scratch after each move.
    new_stack->castlings = board->stack->castlings;
    new_stack->rule50 = board->stack->rule50;
    new_stack->plies_since_nullmove = board->stack->plies_since_nullmove;
    new_stack->ep_square = board->stack->ep_square;
    new_stack->king_pawn_key = board->stack->king_pawn_key;
    new_stack->material_key = board->stack->material_key;
    new_stack->material[WHITE] = board->stack->material[WHITE];
    new_stack->material[BLACK] = board->stack->material[BLACK];

    // Increment the ply-related counters.
    board->ply += 1;
    new_stack->rule50 += 1;
    new_stack->plies_since_nullmove += 1;

    // Special handling for castling rights, since we encode them as "King takes Rook".
    if (move_type(move) == CASTLING) {
        Square rook_from;
        Square rook_to;

        board_do_castling(board, us, from, &to, &rook_from, &rook_to);

        key ^= ZobristPsq[captured_piece][rook_from];
        key ^= ZobristPsq[captured_piece][rook_to];

        captured_piece = NO_PIECE;
    }

    // Additional work is required for captures.
    if (captured_piece) {
        Square capture_square = to;

        if (piece_type(captured_piece) == PAWN) {
            if (move_type(move) == EN_PASSANT) {
                capture_square -= pawn_direction(us);
            }

            new_stack->king_pawn_key ^= ZobristPsq[captured_piece][capture_square];
        } else {
            new_stack->material[them] -= PieceScores[MIDGAME][captured_piece];
        }

        board_remove_piece(board, capture_square);

        // Update the board and material keys, and reset the rule50 counter.
        key ^= ZobristPsq[captured_piece][capture_square];
        new_stack->material_key ^= ZobristPsq[captured_piece][board->piece_count[captured_piece]];
        new_stack->rule50 = 0;
    }

    key ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];

    // Clear the en-passant Zobrist key from the board key if needed.
    if (new_stack->ep_square != SQ_NONE) {
        key ^= ZobristEnPassant[square_file(new_stack->ep_square)];
        new_stack->ep_square = SQ_NONE;
    }

    // If our move modified castling rights, update the board key and the list of available
    // castlings.
    if (new_stack->castlings && (board->castling_mask[from] | board->castling_mask[to])) {
        const CastlingRights lost_castlings = board->castling_mask[from] | board->castling_mask[to];

        key ^= ZobristCastling[new_stack->castlings & lost_castlings];
        new_stack->castlings &= ~lost_castlings;
    }

    // Don't move the piece during castling, since we already do this work in
    // the do_castling() function.
    if (move_type(move) != CASTLING) {
        board_move_piece(board, from, to);
    }

    // Additional work is required for pawn moves.
    if (piece_type(piece) == PAWN) {
        // Update the Pawn key and clear the rule50 counter.
        new_stack->king_pawn_key ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];
        new_stack->rule50 = 0;

        // Set the en-passant square if needed.
        if ((to ^ from) == 16
            && (pawn_attacks_bb(to - pawn_direction(us), us) & board_piece_bb(board, them, PAWN))) {
            new_stack->ep_square = to - pawn_direction(us);
            key ^= ZobristEnPassant[square_file(new_stack->ep_square)];
        } else if (move_type(move) == PROMOTION) {
            const Piece new_piece = create_piece(us, move_promotion_type(move));

            board_remove_piece(board, to);
            board_put_piece(board, new_piece, to);

            key ^= ZobristPsq[piece][to] ^ ZobristPsq[new_piece][to];
            new_stack->king_pawn_key ^= ZobristPsq[piece][to];
            new_stack->material[us] += PieceScores[MIDGAME][move_promotion_type(move)];
            new_stack->material_key ^= ZobristPsq[new_piece][board->piece_count[new_piece] - 1];
            new_stack->material_key ^= ZobristPsq[piece][board->piece_count[piece]];
        }
    } else if (piece_type(piece) == KING) {
        new_stack->king_pawn_key ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];
    }

    new_stack->captured_piece = captured_piece;
    new_stack->board_key = key;

    // Prefetch the TT entry as early as possible.
    // prefetch(tt_entry_at(key));

    // Save the list of checking pieces if the move gives check.
    new_stack->checkers = gives_check
        ? board_attackers_to(board, board_king_square(board, them)) & board_color_bb(board, us)
        : 0;

    board->side_to_move = them;
    boardstack_set_check_info(new_stack, board);
    new_stack->repetition = 0;

    // Link the new stack to the existing list.
    new_stack->previous = board->stack;

    const u16 repetition_plies = u16_min(new_stack->rule50, new_stack->plies_since_nullmove);

    // Only check for 2-fold or 3-fold repetitions when the last irreversible move is at least 4
    // plies away.
    if (repetition_plies >= 4) {
        Boardstack *rewind = new_stack->previous->previous;

        for (u16 i = 4; i <= repetition_plies; i += 2) {
            rewind = rewind->previous->previous;

            if (rewind->board_key == new_stack->board_key) {
                new_stack->repetition = rewind->repetition ? -(i16)i : (i16)i;
                break;
            }
        }
    }

    board->stack = new_stack;
}

void board_do_null_move(Board *restrict board, Boardstack *restrict new_stack) {
    // Copy the whole stack state. (NOTE: this part might be further optimized by only copying the
    // required fields as in the do_move_gc() function, this has not been tested yet.)
    memcpy(new_stack, board->stack, sizeof(Boardstack));

    new_stack->board_key ^= ZobristSideToMove;

    // Prefetch the TT entry as early as possible.
    // prefetch(tt_entry_at(new_stack->board_key));

    // Update the ply-related counters.
    new_stack->rule50 += 1;
    new_stack->plies_since_nullmove = 0;

    // Clear the en-passant Zobrist key from the board key if needed.
    if (new_stack->ep_square != SQ_NONE) {
        new_stack->board_key ^= ZobristEnPassant[square_file(new_stack->ep_square)];
        new_stack->ep_square = SQ_NONE;
    }

    board->side_to_move = color_flip(board->side_to_move);
    new_stack->repetition = 0;
    boardstack_set_check_info(new_stack, board);

    // Link the new stack to the existing list.
    new_stack->previous = board->stack;
    board->stack = new_stack;
}

static void board_undo_castling(
    Board *restrict board,
    Color us,
    Square king_from,
    Square *restrict king_to
) {
    // Get the castling side based on whether the Rook is on the left or the right side of the
    // King.
    const bool kingside = *king_to > king_from;

    // Since we encode castling moves as "King takes Rook", the Rook initial position corresponds
    // to the arrival square of the move. We then compute the real arrival squares of the King and
    // Rook based on the castling side.
    const Square rook_from = *king_to;
    const Square rook_to = square_relative(kingside ? SQ_F1 : SQ_D1, us);

    *king_to = square_relative(kingside ? SQ_G1 : SQ_C1, us);

    // Update the King and Rook positions on the board.
    board_remove_piece(board, *king_to);
    board_remove_piece(board, rook_to);
    board_put_piece(board, create_piece(us, KING), king_from);
    board_put_piece(board, create_piece(us, ROOK), rook_from);
}

void board_undo_move(Board *board, Move move) {
    const Color us = color_flip(board->side_to_move);
    const Square from = move_from(move);
    Square to = move_to(move);

    board->side_to_move = us;

    // If the move was a promotion, place the Pawn back.
    if (move_type(move) == PROMOTION) {
        board_remove_piece(board, to);
        board_put_piece(board, create_piece(us, PAWN), to);
    }

    // Special handling is required for castling moves.
    if (move_type(move) == CASTLING) {
        board_undo_castling(board, us, from, &to);
    } else {
        board_move_piece(board, to, from);

        // If we had captured a piece, place it back on the board.
        if (board->stack->captured_piece) {
            Square capture_square = to;

            if (move_type(move) == EN_PASSANT) {
                capture_square -= pawn_direction(us);
            }

            board_put_piece(board, board->stack->captured_piece, capture_square);
        }
    }

    // Unlink the last stack, and decrement the ply counter.
    board->stack = board->stack->previous;
    board->ply -= 1;
}

void board_undo_null_move(Board *board) {
    board->stack = board->stack->previous;
    board->side_to_move = color_flip(board->side_to_move);
}

bool board_game_is_drawn(const Board *board, u16 ply) {
    // Check if we have a draw by 50-move rule.
    if (board->stack->rule50 > 99) {
        // If the side to move isn't in check, we can claim a draw.
        if (!board->stack->checkers) {
            return true;
        }

        Movelist movelist;

        movelist_generate_legal(&movelist, board);

        if (movelist_size(&movelist) != 0) {
            return true;
        }
    }

    // If we have a repetition that comes from the search state (and not from the given UCI position
    // history), we can claim a draw. This means that we check for 2-fold repetitions coming from
    // solely the search root position and later, and for 3-fold if there are older 2-fold
    // repetitions in the position history.
    return board->stack->repetition && board->stack->repetition < (i16)ply;
}

bool board_game_contains_cycle(const Board *board, u16 ply) {
    const u16 max_plies = u16_min(board->stack->rule50, board->stack->plies_since_nullmove);
    const Key original_key = board->stack->board_key;
    const Boardstack *stack_it = board->stack->previous;

    // If we have less than 3 plies without an irreversible move or a null move, we are guaranteed
    // to not find a cycle in the position.
    if (max_plies < 3) {
        return false;
    }

    for (u16 i = 3; i <= max_plies; i += 2) {
        // Only check for cycles from a single side.
        stack_it = stack_it->previous->previous;

        const Key move_key = original_key ^ stack_it->board_key;

        // Check if the move key corresponds to a reversible move.
        u16 index = cyclic_index_lo(move_key);

        if (CyclicKeys[index] != move_key) {
            index = cyclic_index_hi(move_key);

            if (CyclicKeys[index] != move_key) {
                continue;
            }
        }

        const Move move = CyclicMoves[index];
        const Square from = move_from(move);
        const Square to = move_to(move);

        // Check if there are no pieces between the 'from' and 'to' square.
        if (between_squares_bb(from, to) & board_occupancy_bb(board)) {
            continue;
        }

        // If the cycle is contained in the search tree, return true.
        if (ply > i) {
            return true;
        }

        // For nodes before the search tree or at the root, we check if the move created a
        // repetition earlier (in the same spirit that we check for 2-fold repetitions in the search
        // tree, or 3-fold for repetitions prior to root).
        if (!stack_it->repetition) {
            continue;
        }

        // Verify that we're making the cycle with our move. (This is only necessary for repetitions
        // prior to root).
        if (piece_color(board_piece_on(board, board_square_is_empty(board, from) ? to : from))
            != board->side_to_move) {
            return true;
        }
    }

    return false;
}

bool board_see_above(const Board *board, Move move, Score threshold) {
    static const Score SeeScores[PIECETYPE_NB] = {
        0,
        PAWN_SEE_SCORE,
        KNIGHT_SEE_SCORE,
        BISHOP_SEE_SCORE,
        ROOK_SEE_SCORE,
        QUEEN_SEE_SCORE,
        0,
        0,
    };

    // "Non-standard" moves are tricky to evaluate, so consider them as always having a SEE score of
    // zero. Note that for now we don't count promotions as having a higher SEE from the "material
    // gain" of replacing the pawn with a stronger piece.
    if (move_type(move) != NORMAL_MOVE && move_type(move) != PROMOTION) {
        return threshold <= 0;
    }

    const Square from = move_from(move);
    const Square to = move_to(move);
    Score next_value = SeeScores[piece_type(board_piece_on(board, to))] - threshold;

    // If we can't get enough material with a piece capture, we fail.
    if (next_value < 0) {
        return false;
    }

    next_value = SeeScores[piece_type(board_piece_on(board, from))] - next_value;

    // Likewise, if our opponent can't get enough material with a piece capture, we pass.
    if (next_value <= 0) {
        return true;
    }

    Bitboard occupancy = board_occupancy_bb(board) ^ square_bb(from) ^ square_bb(to);
    Color side_to_move = piece_color(board_piece_on(board, from));
    Bitboard attackers = board_attackers_list(board, to, occupancy);
    bool result = true;

    // Perform exchanges on the target square, until one side has no attackers left or fails to pass
    // the material threshold with its next capture.

    while (true) {
        side_to_move = color_flip(side_to_move);
        attackers &= occupancy;

        Bitboard stm_attackers = attackers & board_color_bb(board, side_to_move);
        Bitboard weakest_attackers;

        // Stop the loop if one of the sides has no attackers left.
        if (!stm_attackers) {
            break;
        }

        // Exclude pinned pieces from the list of available attackers.
        if (board->stack->pinners[color_flip(side_to_move)] & occupancy) {
            stm_attackers &= ~board->stack->king_blockers[side_to_move];

            if (!stm_attackers) {
                break;
            }
        }

        result = !result;

        // Use the pieces for capturing in the ascending order of their material values. Update the
        // attackers' list as pieces get removed to include X-ray attacks from Bishops/Rooks/Queens
        // being lined up on the target square.
        if ((weakest_attackers = stm_attackers & board_piecetype_bb(board, PAWN))) {
            next_value = PAWN_SEE_SCORE - next_value;

            if (next_value < (Score)result) {
                break;
            }

            // Equivalent to bb_reset_square(occupancy, bb_first_square(weakest_attackers)).
            occupancy ^= weakest_attackers & -weakest_attackers;
            attackers |=
                bishop_attacks_bb(to, occupancy) & board_piecetypes_bb(board, BISHOP, QUEEN);
        } else if ((weakest_attackers = stm_attackers & board_piecetype_bb(board, KNIGHT))) {
            next_value = KNIGHT_SEE_SCORE - next_value;

            if (next_value < (Score)result) {
                break;
            }

            occupancy ^= weakest_attackers & -weakest_attackers;
        } else if ((weakest_attackers = stm_attackers & board_piecetype_bb(board, BISHOP))) {
            next_value = BISHOP_SEE_SCORE - next_value;

            if (next_value < (Score)result) {
                break;
            }

            occupancy ^= weakest_attackers & -weakest_attackers;
            attackers |=
                bishop_attacks_bb(to, occupancy) & board_piecetypes_bb(board, BISHOP, QUEEN);
        } else if ((weakest_attackers = stm_attackers & board_piecetype_bb(board, ROOK))) {
            next_value = ROOK_SEE_SCORE - next_value;

            if (next_value < (Score)result) {
                break;
            }

            occupancy ^= weakest_attackers & -weakest_attackers;
            attackers |= rook_attacks_bb(to, occupancy) & board_piecetypes_bb(board, ROOK, QUEEN);
        } else if ((weakest_attackers = stm_attackers & board_piecetype_bb(board, QUEEN))) {
            next_value = QUEEN_SEE_SCORE - next_value;

            if (next_value < (Score)result) {
                break;
            }

            occupancy ^= weakest_attackers & -weakest_attackers;
            attackers |=
                bishop_attacks_bb(to, occupancy) & board_piecetypes_bb(board, BISHOP, QUEEN);
            attackers |= rook_attacks_bb(to, occupancy) & board_piecetypes_bb(board, ROOK, QUEEN);
        } else {
            return (attackers & ~board_color_bb(board, side_to_move)) ? !result : result;
        }
    }

    return result;
}

StringView board_move_to_uci(const Board *board, Move move) {
    static u8 move_buffer[5];

    if (move == NO_MOVE) {
        memcpy(move_buffer, "none", 4);
        return strview_from_raw_data(move_buffer, 4);
    }

    if (move == NULL_MOVE) {
        memcpy(move_buffer, "0000", 4);
        return strview_from_raw_data(move_buffer, 4);
    }

    const Square from = move_from(move);
    Square to = move_to(move);

    if (move_type(move) == CASTLING && !board->chess960) {
        to = create_square(to > from ? FILE_G : FILE_C, square_rank(from));
    }

    move_buffer[0] = square_file(from) + 'a';
    move_buffer[1] = square_rank(from) + '1';
    move_buffer[2] = square_file(to) + 'a';
    move_buffer[3] = square_rank(to) + '1';

    if (move_type(move) == PROMOTION) {
        move_buffer[4] = " pnbrqk"[move_promotion_type(move)];
        return strview_from_raw_data(move_buffer, 5);
    }

    return strview_from_raw_data(move_buffer, 4);
}

static bool is_valid_file_ascii(u8 b) {
    return b >= 'a' && b <= 'h';
}

static bool is_valid_rank_ascii(u8 b) {
    return b >= '1' && b <= '8';
}

static bool board_move_tries_castling(const Board *board, Square from, Square to) {
    const Piece moving_piece = board_piece_on(board, from);

    if (piece_type(moving_piece) != KING) {
        return false;
    }

    if (board->chess960) {
        // Simple check for Chess960: UCI castling moves are also encoded as "King takes Rook".
        const Piece captured = board_piece_on(board, to);

        return piece_type(captured) == ROOK && piece_color(moving_piece) == piece_color(captured);
    }

    // For standard chess, simply check if the starting file and arrival file corresponds to a
    // castling move, and let the pseudo-legal check verify the rest.
    return square_file(from) == FILE_E && (square_file(to) == FILE_C || square_file(to) == FILE_G);
}

static bool board_move_tries_enpassant(const Board *board, Square from, Square to) {
    const Piece moving_piece = board_piece_on(board, from);

    if (board->stack->ep_square == SQ_NONE) {
        return false;
    }

    return piece_type(moving_piece) == PAWN && to == board->stack->ep_square;
}

Move board_uci_to_move(const Board *board, StringView move_strview) {
    if (move_strview.size != 4 && move_strview.size != 5) {
        return NO_MOVE;
    }

    const u8 from_file = move_strview.data[0];
    const u8 from_rank = move_strview.data[1];
    const u8 to_file = move_strview.data[2];
    const u8 to_rank = move_strview.data[3];

    if (!is_valid_file_ascii(from_file) || !is_valid_rank_ascii(from_rank)
        || !is_valid_file_ascii(to_file) || !is_valid_rank_ascii(to_rank)) {
        return NO_MOVE;
    }

    const Square from = create_square(from_file - 'a', from_rank - '1');
    const Square to = create_square(to_file - 'a', to_rank - '1');
    Move move;

    // Special handling for promotions.
    if (move_strview.size == 5) {
        u8 promo_char = move_strview.data[4];

        // Some GUIs/scripting tools might send the promotion as an uppercase letter.
        if (promo_char >= 'A' && promo_char <= 'Z') {
            promo_char = promo_char - 'A' + 'a';
        }

        if (promo_char < 'a' || promo_char > 'z') {
            return NO_MOVE;
        }

        usize promotion_index = strview_find(PromotionIndexes, promo_char);

        if (promotion_index == NPOS) {
            return NO_MOVE;
        }

        move = create_promotion_move(from, to, (Piece)promotion_index);
    }
    // Special handling for castling moves.
    else if (board_move_tries_castling(board, from, to)) {
        Square king_to =
            board->chess960 ? to : create_square(from < to ? FILE_H : FILE_A, square_rank(to));

        move = create_castling_move(from, king_to);
    }
    // Special handling for en-passant moves.
    else if (board_move_tries_enpassant(board, from, to)) {
        move = create_en_passant_move(from, to);
    } else {
        move = create_move(from, to);
    }

    if (!board_move_is_pseudolegal(board, move) || !board_move_is_legal(board, move)) {
        return NO_MOVE;
    }

    return move;
}
