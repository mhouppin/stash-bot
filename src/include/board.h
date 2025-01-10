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

#ifndef BOARD_H
#define BOARD_H

#include "bitboard.h"
#include "chess_types.h"
#include "core.h"
#include "hashkey.h"
#include "strview.h"

// Struct representing the board stack data from past moves
typedef struct _Boardstack {
    Key board_key;
    Key king_pawn_key;
    Key material_key;
    Bitboard checkers;
    Bitboard king_blockers[COLOR_NB];
    Bitboard pinners[COLOR_NB];
    Bitboard check_squares[PIECETYPE_NB];
    struct _Boardstack *previous;
    CastlingRights castlings;
    u16 rule50;
    i16 repetition;
    u16 plies_since_nullmove;
    Square ep_square;
    Piece captured_piece;
    Score material[COLOR_NB];
} Boardstack;

// Struct representing the board
typedef struct _Board {
    Piece mailbox[SQUARE_NB];
    Bitboard piecetype_bb[PIECETYPE_NB];
    Bitboard color_bb[COLOR_NB];
    u8 piece_count[PIECE_NB];
    CastlingRights castling_mask[SQUARE_NB];
    Square castling_rook_square[CASTLING_NB];
    Bitboard castling_path[CASTLING_NB];
    Boardstack *stack;
    u16 ply;
    Color side_to_move;
    Scorepair psq_scorepair;
    bool chess960;
    bool has_worker;
} Board;

extern const StringView PieceIndexes;
extern const StringView StartposStr;

// Initializes cycle detection tables
void cyclic_init(void);

// Initializes the board stack from the given board
void boardstack_init(Boardstack *restrict stack, const Board *restrict board);

// Duplicates the board stack (including all the linked sub-stacks)
Boardstack *boardstack_clone(const Boardstack *stack);

// Frees memory owned by the board stack (including all the linked sub-stacks)
void boardstack_destroy(Boardstack *stack);

// Initializes the board from the given FEN string. Returns true if the board
// was correctly initialized, false otherwise.
bool board_try_init(Board *board, StringView fen, bool is_chess960, Boardstack *stack);

// Clones a board and its stack into another board struct
void board_clone(Board *restrict board, const Board *restrict other);

// Returns the FEN representation of the board
StringView board_get_fen(const Board *board);

// Checks if the given move is pseudo-legal
bool board_move_is_pseudolegal(const Board *board, Move move);

// Checks if the given pseudo-legal move is legal
bool board_move_is_legal(const Board *board, Move move);

// Checks if the given move gives check
bool board_move_gives_check(const Board *board, Move move);

// Applies a legal move to the board, using the extra check info for faster execution
void board_do_move_gc(
    Board *restrict board,
    Move move,
    Boardstack *restrict new_stack,
    bool gives_check
);

// Applies a legal move to the board
INLINED void board_do_move(Board *restrict board, Move move, Boardstack *restrict new_stack) {
    board_do_move_gc(board, move, new_stack, board_move_gives_check(board, move));
}

// Applies a null move to the board
void board_do_null_move(Board *restrict board, Boardstack *restrict new_stack);

// Reverts the given move
void board_undo_move(Board *board, Move move);

// Reverts a null move
void board_undo_null_move(Board *board);

// Checks if the game is drawn by 50-move rule or by repetition
bool board_game_is_drawn(const Board *board, u16 ply);

// Checks if the opponent created a cycle in the tree, or if we can complete a cycle with a move
bool board_game_contains_cycle(const Board *board, u16 ply);

// Checks if the given move has a Static Exchange Evaluation score greater than or equal to the
// given threshold
bool board_see_above(const Board *board, Move move, Score threshold);

// Converts a move to its UCI string representation
StringView board_move_to_uci(const Board *board, Move move);

// Converts a UCI move representation to our internal move type, or NO_MOVE if no legal move matches
// the given string
Move board_uci_to_move(const Board *board, StringView move_strview);

// Returns the piece on the given square
INLINED Piece board_piece_on(const Board *board, Square square) {
    assert(square_is_valid(square));
    return board->mailbox[square];
}

// Checks if the given square is empty
INLINED bool board_square_is_empty(const Board *board, Square square) {
    assert(square_is_valid(square));
    return board_piece_on(board, square) == NO_PIECE;
}

// Returns the piece performing the given move (before the move is actually done)
INLINED Piece board_moved_piece(const Board *board, Move move) {
    return board_piece_on(board, move_from(move));
}

// Returns a bitboard of all pieces matching the given color
INLINED Bitboard board_color_bb(const Board *board, Color color) {
    assert(color_is_valid(color));
    return board->color_bb[color];
}

// Returns a bitboard of all pieces matching the given piece type
INLINED Bitboard board_piecetype_bb(const Board *board, Piecetype piecetype) {
    assert(piecetype_is_valid(piecetype));
    return board->piecetype_bb[piecetype];
}

// Returns a bitboard of all pieces matching the given piece types
INLINED Bitboard
    board_piecetypes_bb(const Board *board, Piecetype piecetype1, Piecetype piecetype2) {
    assert(piecetype_is_valid(piecetype1));
    assert(piecetype_is_valid(piecetype2));
    return board_piecetype_bb(board, piecetype1) | board_piecetype_bb(board, piecetype2);
}
// Returns a bitboard of all pieces matching the given color and piece type
INLINED Bitboard board_piece_bb(const Board *board, Color color, Piecetype piecetype) {
    assert(color_is_valid(color));
    assert(piecetype_is_valid(piecetype));
    return board_color_bb(board, color) & board_piecetype_bb(board, piecetype);
}

// Returns a bitboard of all pieces matching the given color and piece types
INLINED Bitboard
    board_pieces_bb(const Board *board, Color color, Piecetype piecetype1, Piecetype piecetype2) {
    assert(color_is_valid(color));
    assert(piecetype_is_valid(piecetype1));
    assert(piecetype_is_valid(piecetype2));
    return board_color_bb(board, color) & board_piecetypes_bb(board, piecetype1, piecetype2);
}

// Returns a bitboard of all pieces
INLINED Bitboard board_occupancy_bb(const Board *board) {
    return board_piecetype_bb(board, ALL_PIECES);
}

// Returns the number of pieces matching the color and piecetype on the board
INLINED u8 board_piece_count(const Board *board, Piece piece) {
    assert(piece_is_valid(piece));
    return board->piece_count[piece];
}

INLINED u8 board_piecetype_count(const Board *board, Piecetype piecetype) {
    assert(piecetype_is_valid(piecetype));
    return board_piece_count(board, create_piece(WHITE, piecetype))
        + board_piece_count(board, create_piece(BLACK, piecetype));
}

// Returns the number of pieces matching the color on the board
INLINED u8 board_color_count(const Board *board, Color color) {
    assert(color_is_valid(color));
    return board_piece_count(board, create_piece(color, ALL_PIECES));
}

// Returns the number of pieces on the board
INLINED u8 board_total_piece_count(const Board *board) {
    return board_color_count(board, WHITE) + board_color_count(board, BLACK);
}

// Returns the king square of the given color
INLINED Bitboard board_king_square(const Board *board, Color color) {
    assert(color_is_valid(color));
    return bb_first_square(board_piece_bb(board, color, KING));
}

// Checks if the given castling has an obstructed path
INLINED bool board_castling_is_blocked(const Board *board, CastlingRights castling) {
    return !!(board_occupancy_bb(board) & board->castling_path[castling]);
}

// Checks if the given move is a capture or a promotion
INLINED bool board_move_is_noisy(const Board *board, Move move) {
    return move_type(move) == NORMAL_MOVE ? !board_square_is_empty(board, move_to(move))
                                          : move_type(move) != CASTLING;
}

// Helper function for extracting a zobrist hash of the pawn structure
INLINED Key board_pawn_key(const Board *board) {
    return board->stack->king_pawn_key ^ ZobristPsq[WHITE_KING][board_king_square(board, WHITE)]
        ^ ZobristPsq[BLACK_KING][board_king_square(board, BLACK)];
}

// Helper function for grabbing a material count with standardized values: Pawn=1, Knight=Bishop=3,
// Rook=5, Queen=9
INLINED u32 board_material_count(const Board *board) {
    return 9 * board_piecetype_count(board, QUEEN) + 5 * board_piecetype_count(board, ROOK)
        + 3 * board_piecetype_count(board, BISHOP) + 3 * board_piecetype_count(board, KNIGHT)
        + board_piecetype_count(board, PAWN);
}

#endif
