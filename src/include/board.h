/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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
#include "hashkey.h"
#include "psq_score.h"
#include "types.h"

// Struct representing the board stack data from past moves
typedef struct _Boardstack
{
    int castlings;
    int rule50;
    int pliesFromNullMove;
    square_t enPassantSquare;
    hashkey_t pawnKey;
    score_t material[COLOR_NB];
    hashkey_t materialKey;
    hashkey_t boardKey;
    bitboard_t checkers;
    piece_t capturedPiece;
    struct _Boardstack *prev;
    bitboard_t kingBlockers[COLOR_NB];
    bitboard_t pinners[COLOR_NB];
    bitboard_t checkSquares[PIECETYPE_NB];
    int repetition;
} Boardstack;

// Struct representing the board
typedef struct _Board
{
    piece_t table[SQUARE_NB];
    bitboard_t piecetypeBB[PIECETYPE_NB];
    bitboard_t colorBB[COLOR_NB];
    int pieceCount[PIECE_NB];
    int castlingMask[SQUARE_NB];
    square_t castlingRookSquare[CASTLING_NB];
    bitboard_t castlingPath[CASTLING_NB];
    int ply;
    color_t sideToMove;
    scorepair_t psqScorePair;
    Boardstack *stack;
    void *worker;
    bool chess960;
} Board;

extern Board UciBoard;

// Initializes cycle detection tables.
void cyclic_init(void);

// Returns the list of attacking pieces for a given square and occupancy.
bitboard_t attackers_list(const Board *board, square_t s, bitboard_t occupied);

// Helper for applying castling moves.
void do_castling(Board *restrict board, color_t us, square_t kingFrom, square_t *restrict kingTo,
    square_t *restrict rookFrom, square_t *restrict rookTo);

// Applies a legal move to the board. This function needs information about whether
// the move gives check or not, if this information is missing, call do_move() instead.
void do_move_gc(Board *restrict board, move_t move, Boardstack *restrict stack, bool givesCheck);

// Applies a null move to the board. This function must not be called when the
// side to move is in check.
void do_null_move(Board *restrict board, Boardstack *restrict stack);

// Returns the FEN representation of the board.
const char *board_fen(const Board *board);

// Checks if the game is drawn by 50-move rule or by repetition. We check for 2-fold repetition
// for all moves past the given ply, and 3-fold repetition for moves prior to that (to avoid
// penalizing the engine for a 2-fold occuring earlier in the tree).
bool game_is_drawn(const Board *board, int ply);

// Checks if the opponent created a cycle in the tree, or if we can complete a cycle with a move.
// This basically works as an early detection of draws.
bool game_has_cycle(const Board *board, int ply);

// Checks if the given pseudo-legal move is legal.
bool move_is_legal(const Board *board, move_t move);

// Checks if the given move is pseudo-legal.
bool move_is_pseudo_legal(const Board *board, move_t move);

// Checks if the given move gives check.
bool move_gives_check(const Board *board, move_t move);

// Checks if the given move has a Static Exchange Evaluation score greater than or equal to the
// given threshold.
bool see_greater_than(const Board *board, move_t move, score_t threshold);

// Initializes the board from the given FEN string.
int board_from_fen(Board *board, const char *fen, bool isChess960, Boardstack *bstack);

// Initializes the board stack from the given board.
void set_boardstack(Board *board, Boardstack *stack);

// Initializes checkers info (pinned pieces, checking squares, etc.).
void set_check(Board *restrict board, Boardstack *restrict stack);

// Returns the bitboard of all pieces preventing attacks on the given square.
bitboard_t slider_blockers(
    const Board *restrict board, bitboard_t sliders, square_t square, bitboard_t *restrict pinners);

// Helper for reverting castling moves.
void undo_castling(Board *restrict board, color_t us, square_t kingFrom, square_t *restrict kingTo,
    square_t *restrict rookFrom, square_t *restrict rookTo);

// Reverts the given move.
void undo_move(Board *board, move_t move);

// Reverts a null move.
void undo_null_move(Board *board);

// Duplicates a board stack (including all the linked sub-nodes).
Boardstack *dup_boardstack(const Boardstack *stack);

// Frees a board stack (including all the linked sub-nodes).
void free_boardstack(Boardstack *stack);

// Returns the piece on the given square.
INLINED piece_t piece_on(const Board *board, square_t square) { return board->table[square]; }

// Checks if the given square is empty.
INLINED bool empty_square(const Board *board, square_t square)
{
    return piece_on(board, square) == NO_PIECE;
}

// Returns the piece performing the given move.
INLINED piece_t moved_piece(const Board *board, move_t move)
{
    return piece_on(board, from_sq(move));
}

// Returns a bitboard of all pieces matching the given piece type.
INLINED bitboard_t piecetype_bb(const Board *board, piecetype_t pt)
{
    return board->piecetypeBB[pt];
}

// Returns a bitboard of all pieces matching the given piece types.
INLINED bitboard_t piecetypes_bb(const Board *board, piecetype_t pt1, piecetype_t pt2)
{
    return piecetype_bb(board, pt1) | piecetype_bb(board, pt2);
}

// Returns a bitboard of all pieces matching the given color.
INLINED bitboard_t color_bb(const Board *board, color_t color) { return board->colorBB[color]; }

// Returns a bitboard of all pieces matching the given color and piece type.
INLINED bitboard_t piece_bb(const Board *board, color_t color, piecetype_t pt)
{
    return piecetype_bb(board, pt) & color_bb(board, color);
}

// Returns a bitboard of all pieces matching the given color and piece types.
INLINED bitboard_t pieces_bb(const Board *board, color_t color, piecetype_t pt1, piecetype_t pt2)
{
    return piecetypes_bb(board, pt1, pt2) & color_bb(board, color);
}

// Returns a bitboard of all pieces.
INLINED bitboard_t occupancy_bb(const Board *board) { return piecetype_bb(board, ALL_PIECES); }

// Returns the king square of the given color.
INLINED bitboard_t get_king_square(const Board *board, color_t color)
{
    return bb_first_sq(piece_bb(board, color, KING));
}

// Returns a bitboard of all pseudo-legal king moves for the given square.
INLINED bitboard_t king_moves(square_t square) { return PseudoMoves[KING][square]; }

// Returns a bitboard of all pseudo-legal knight moves for the given square.
INLINED bitboard_t knight_moves(square_t square) { return PseudoMoves[KNIGHT][square]; }

// Returns a bitboard of all pseudo-legal pawn moves for the given square and color.
INLINED bitboard_t pawn_moves(square_t square, color_t color) { return PawnMoves[color][square]; }

// Returns a bitboard of all pseudo-legal bishop moves for the given square.
INLINED bitboard_t bishop_moves(const Board *board, square_t square)
{
    return bishop_moves_bb(square, occupancy_bb(board));
}

// Returns a bitboard of all pseudo-legal rook moves for the given square.
INLINED bitboard_t rook_moves(const Board *board, square_t square)
{
    return rook_moves_bb(square, occupancy_bb(board));
}

// Returns a bitboard of all pseudo-legal queen moves for the given square.
INLINED bitboard_t queen_moves(const Board *board, square_t square)
{
    return bishop_moves(board, square) | rook_moves(board, square);
}

// Returns a bitboard of all pseudo-legal piece moves for the given piece type, square and
// occupancy.
INLINED bitboard_t piece_moves(piecetype_t piecetype, square_t square, bitboard_t occupied)
{
    switch (piecetype)
    {
        case KNIGHT: return knight_moves(square);
        case BISHOP: return bishop_moves_bb(square, occupied);
        case ROOK: return rook_moves_bb(square, occupied);
        case QUEEN: return bishop_moves_bb(square, occupied) | rook_moves_bb(square, occupied);
        case KING: return king_moves(square);

        default: __builtin_unreachable(); return 0;
    }
}

// Returns a bitboard of all pieces attacking the given square.
INLINED bitboard_t attackers_to(const Board *board, square_t square)
{
    return attackers_list(board, square, occupancy_bb(board));
}

// Checks if the given castling is blocked by pieces on the castling path.
INLINED bool castling_blocked(const Board *board, int castling)
{
    return occupancy_bb(board) & board->castlingPath[castling];
}

// Checks if the given move is a capture or a promotion.
INLINED bool is_capture_or_promotion(const Board *board, move_t move)
{
    return move_type(move) == NORMAL_MOVE ? !empty_square(board, to_sq(move))
                                          : move_type(move) != CASTLING;
}

// Helper function for putting pieces on the board.
INLINED void put_piece(Board *board, piece_t piece, square_t square)
{
    bitboard_t sqbb = square_bb(square);

    board->table[square] = piece;
    board->piecetypeBB[ALL_PIECES] |= sqbb;
    board->piecetypeBB[piece_type(piece)] |= sqbb;
    board->colorBB[piece_color(piece)] |= sqbb;
    board->pieceCount[piece]++;
    board->pieceCount[create_piece(piece_color(piece), ALL_PIECES)]++;
    board->psqScorePair += PsqScore[piece][square];
}

// Helper function for moving pieces on the board.
INLINED void move_piece(Board *board, square_t from, square_t to)
{
    piece_t piece = piece_on(board, from);
    bitboard_t moveBB = square_bb(from) | square_bb(to);

    board->piecetypeBB[ALL_PIECES] ^= moveBB;
    board->piecetypeBB[piece_type(piece)] ^= moveBB;
    board->colorBB[piece_color(piece)] ^= moveBB;
    board->table[from] = NO_PIECE;
    board->table[to] = piece;
    board->psqScorePair += PsqScore[piece][to] - PsqScore[piece][from];
}

// Helper function for taking pieces off the board.
INLINED void remove_piece(Board *board, square_t square)
{
    piece_t piece = piece_on(board, square);
    bitboard_t sqbb = square_bb(square);

    board->piecetypeBB[ALL_PIECES] ^= sqbb;
    board->piecetypeBB[piece_type(piece)] ^= sqbb;
    board->colorBB[piece_color(piece)] ^= sqbb;
    board->pieceCount[piece]--;
    board->pieceCount[create_piece(piece_color(piece), ALL_PIECES)]--;
    board->psqScorePair -= PsqScore[piece][square];
}

// Applies a legal move to the board.
INLINED void do_move(Board *restrict board, move_t move, Boardstack *restrict stack)
{
    do_move_gc(board, move, stack, move_gives_check(board, move));
}

#endif // BOARD_H
