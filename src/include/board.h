/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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
# define BOARD_H

# include <stdbool.h>
# include "bitboard.h"
# include "castling.h"
# include "color.h"
# include "hashkey.h"
# include "inlining.h"
# include "move.h"
# include "piece.h"
# include "square.h"
# include "psq_score.h"

typedef struct boardstack_s
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
    struct boardstack_s *prev;
    bitboard_t kingBlockers[COLOR_NB];
    bitboard_t pinners[COLOR_NB];
    bitboard_t checkSquares[PIECETYPE_NB];
    int repetition;
}
boardstack_t;

typedef struct board_s
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
    boardstack_t *stack;
    void *worker;
    bool chess960;
}
board_t;

extern board_t Board;

void cyclic_init(void);
bitboard_t attackers_list(const board_t *board, square_t s, bitboard_t occupied);
void do_castling(board_t *board, color_t us, square_t kingFrom, square_t *kingTo,
    square_t *rookFrom, square_t *rookTo);
void do_move_gc(board_t *board, move_t move, boardstack_t *stack, bool givesCheck);
void do_null_move(board_t *board, boardstack_t *stack);
const char *board_fen(const board_t *board);
bool game_is_drawn(const board_t *board, int ply);
bool game_has_cycle(const board_t *board, int ply);
bool move_is_legal(const board_t *board, move_t move);
bool move_is_pseudo_legal(const board_t *board, move_t move);
bool move_gives_check(const board_t *board, move_t move);
bool see_greater_than(const board_t *board, move_t move, score_t threshold);
void set_board(board_t *board, char *fen, bool isChess960, boardstack_t *bstack);
void set_boardstack(board_t *board, boardstack_t *stack);
void set_castling(board_t *board, color_t color, square_t rookSquare);
void set_check(board_t *board, boardstack_t *stack);
bitboard_t slider_blockers(const board_t *board, bitboard_t sliders, square_t square,
    bitboard_t *pinners);
void undo_castling(board_t *board, color_t us, square_t kingFrom,
    square_t *kingTo, square_t *rookFrom, square_t *rookTo);
void undo_move(board_t *board, move_t move);
void undo_null_move(board_t *board);

boardstack_t *dup_boardstack(const boardstack_t *stack);
void free_boardstack(boardstack_t *stack);

INLINED piece_t piece_on(const board_t *board, square_t square)
{
    return (board->table[square]);
}

INLINED bool empty_square(const board_t *board, square_t square)
{
    return (piece_on(board, square) == NO_PIECE);
}

INLINED piece_t moved_piece(const board_t *board, move_t move)
{
    return (piece_on(board, from_sq(move)));
}

INLINED bitboard_t piecetype_bb(const board_t *board, piecetype_t pt)
{
    return (board->piecetypeBB[pt]);
}

INLINED bitboard_t piecetypes_bb(const board_t *board, piecetype_t pt1, piecetype_t pt2)
{
    return (piecetype_bb(board, pt1) | piecetype_bb(board, pt2));
}

INLINED bitboard_t color_bb(const board_t *board, color_t color)
{
    return (board->colorBB[color]);
}

INLINED bitboard_t piece_bb(const board_t *board, color_t color, piecetype_t pt)
{
    return (piecetype_bb(board, pt) & color_bb(board, color));
}

INLINED bitboard_t pieces_bb(const board_t *board, color_t color, piecetype_t pt1, piecetype_t pt2)
{
    return (piecetypes_bb(board, pt1, pt2) & color_bb(board, color));
}

INLINED bitboard_t occupancy_bb(const board_t *board)
{
    return (piecetype_bb(board, ALL_PIECES));
}

INLINED bitboard_t get_king_square(const board_t *board, color_t color)
{
    return (bb_first_sq(piece_bb(board, color, KING)));
}

INLINED bitboard_t king_moves(square_t square)
{
    return (PseudoMoves[KING][square]);
}

INLINED bitboard_t knight_moves(square_t square)
{
    return (PseudoMoves[KNIGHT][square]);
}

INLINED bitboard_t pawn_moves(square_t square, color_t color)
{
    return (PawnMoves[color][square]);
}

INLINED bitboard_t bishop_moves(const board_t *board, square_t square)
{
    return (bishop_moves_bb(square, occupancy_bb(board)));
}

INLINED bitboard_t rook_moves(const board_t *board, square_t square)
{
    return (rook_moves_bb(square, occupancy_bb(board)));
}

INLINED bitboard_t queen_moves(const board_t *board, square_t square)
{
    return (bishop_moves(board, square) | rook_moves(board, square));
}

INLINED bitboard_t piece_moves(piecetype_t piecetype, square_t square, bitboard_t occupied)
{
    switch (piecetype)
    {
        case KNIGHT: return (knight_moves(square));
        case BISHOP: return (bishop_moves_bb(square, occupied));
        case ROOK:   return (rook_moves_bb(square, occupied));
        case QUEEN:  return (bishop_moves_bb(square, occupied) | rook_moves_bb(square, occupied));
        case KING:   return (king_moves(square));

        default:
            __builtin_unreachable();
            return (0);
    }
}

INLINED bitboard_t attackers_to(const board_t *board, square_t square)
{
    return (attackers_list(board, square, occupancy_bb(board)));
}

INLINED bool castling_blocked(const board_t *board, int castling)
{
    return (occupancy_bb(board) & board->castlingPath[castling]);
}

INLINED bool is_capture_or_promotion(const board_t *board, move_t move)
{
    return (move_type(move) == NORMAL_MOVE
        ? !empty_square(board, to_sq(move))
        : move_type(move) != CASTLING);
}

INLINED void put_piece(board_t *board, piece_t piece, square_t square)
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

INLINED void move_piece(board_t *board, square_t from, square_t to)
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

INLINED void remove_piece(board_t *board, square_t square)
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

INLINED void do_move(board_t *board, move_t move, boardstack_t *stack)
{
    do_move_gc(board, move, stack, move_gives_check(board, move));
}

#endif
