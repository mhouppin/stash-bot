/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

typedef struct boardstack_s boardstack_t;

struct  boardstack_s
{
    int             castlings;
    int             rule50;
    int             plies_from_null_move;
    square_t        en_passant_square;
    hashkey_t       board_key;
    hashkey_t       pawn_key;
    score_t         material[COLOR_NB];
    bitboard_t      checkers;
    piece_t         captured_piece;
    boardstack_t    *prev;
    bitboard_t      king_blockers[COLOR_NB];
    bitboard_t      pinners[COLOR_NB];
    bitboard_t      check_squares[PIECETYPE_NB];
    int             repetition;
};

typedef struct  board_s
{
    piece_t         table[SQUARE_NB];
    bitboard_t      piecetype_bits[PIECETYPE_NB];
    bitboard_t      color_bits[COLOR_NB];
    int             piece_count[PIECE_NB];
    int             castling_mask[SQUARE_NB];
    square_t        castling_rook_square[CASTLING_NB];
    bitboard_t      castling_path[CASTLING_NB];
    int             ply;
    color_t         side_to_move;
    scorepair_t     psq_scorepair;
    boardstack_t    *stack;
    void            *worker;
    bool            chess960;
}                board_t;

bool            board_legal(const board_t *board, move_t move);
bool            move_gives_check(const board_t *board, move_t move);

bitboard_t      attackers_list(const board_t *board, square_t s,
                bitboard_t occupied);

bitboard_t      slider_blockers(const board_t *board, bitboard_t sliders,
                square_t square, bitboard_t *pinners);

void            set_castling(board_t *board, color_t color,
                square_t rook_square);

void            set_boardstack(board_t *board, boardstack_t *stack);
void            set_check(board_t *board, boardstack_t *stack);

void            board_set(board_t *board, char *fen, bool is_chess960,
                boardstack_t *bstack);

void            undo_move(board_t *board, move_t move);
void            do_move_gc(board_t *board, move_t move, boardstack_t *stack,
                bool gives_check);

void            do_castling(board_t *board, color_t us, square_t king_from,
                square_t *king_to, square_t *rook_from, square_t *rook_to);

void            undo_castling(board_t *board, color_t us, square_t king_from,
                square_t *king_to, square_t *rook_from, square_t *rook_to);

void            do_null_move(board_t *board, boardstack_t *stack);
void            undo_null_move(board_t *board);

bool            is_draw(const board_t *board, int ply);

bool            see_greater_than(const board_t *board, move_t move, score_t threshold);

boardstack_t    *boardstack_dup(const boardstack_t *stack);
void            boardstack_free(boardstack_t *stack);

INLINED bool    empty_square(const board_t *board, square_t square)
{
    return (board->table[square] == NO_PIECE);
}

INLINED piece_t piece_on(const board_t *board, square_t square)
{
    return (board->table[square]);
}

INLINED piece_t moved_piece(const board_t *board, move_t move)
{
    return (board->table[move_from_square(move)]);
}

INLINED bitboard_t  board_king_square(const board_t *board, color_t color)
{
    return (first_square(board->piecetype_bits[KING] & board->color_bits[color]));
}

INLINED bitboard_t  piecetype_bb(const board_t *board, piecetype_t pt)
{
    return (board->piecetype_bits[pt]);
}

INLINED bitboard_t  piecetypes_bb(const board_t *board, piecetype_t pt1,
                    piecetype_t pt2)
{
    return (board->piecetype_bits[pt1] | board->piecetype_bits[pt2]);
}

INLINED bitboard_t  piece_bb(const board_t *board, color_t color, piecetype_t pt)
{
    return (board->piecetype_bits[pt] & board->color_bits[color]);
}

INLINED bitboard_t  pieces_bb(const board_t *board, color_t color,
                    piecetype_t pt1, piecetype_t pt2)
{
    return ((board->piecetype_bits[pt1] | board->piecetype_bits[pt2])
        & board->color_bits[color]);
}

INLINED bitboard_t  king_moves(square_t square)
{
    return (PseudoMoves[KING][square]);
}

INLINED bitboard_t  knight_moves(square_t square)
{
    return (PseudoMoves[KNIGHT][square]);
}

INLINED bitboard_t  pawn_moves(square_t square, color_t color)
{
    return (PawnMoves[color][square]);
}

INLINED bitboard_t  bishop_moves(const board_t *board, square_t square)
{
    return (bishop_move_bits(square, board->piecetype_bits[ALL_PIECES]));
}

INLINED bitboard_t  rook_moves(const board_t *board, square_t square)
{
    return (rook_move_bits(square, board->piecetype_bits[ALL_PIECES]));
}

INLINED bitboard_t  queen_moves(const board_t *board, square_t square)
{
    return (bishop_move_bits(square, board->piecetype_bits[ALL_PIECES])
        | rook_move_bits(square, board->piecetype_bits[ALL_PIECES]));
}

INLINED bitboard_t  piece_moves(piecetype_t piecetype, square_t square,
                    bitboard_t occupied)
{
    switch (piecetype)
    {
        case KNIGHT:
            return (PseudoMoves[KNIGHT][square]);

        case BISHOP:
            return (bishop_move_bits(square, occupied));

        case ROOK:
            return (rook_move_bits(square, occupied));

        case QUEEN:
            return (bishop_move_bits(square, occupied)
                | rook_move_bits(square, occupied));

        default:
            __builtin_unreachable();
            return (0);
    }
}

INLINED bitboard_t  attackers_to(const board_t *board, square_t square)
{
    return (attackers_list(board, square, board->piecetype_bits[ALL_PIECES]));
}

INLINED bool        castling_blocked(const board_t *board, int castling)
{
    return (board->piecetype_bits[ALL_PIECES] & board->castling_path[castling]);
}

INLINED bool        is_capture_or_promotion(const board_t *board, move_t move)
{
    return (type_of_move(move) == NORMAL_MOVE ? !empty_square(board, move_to_square(move))
        : type_of_move(move) != CASTLING);
}

INLINED void        put_piece(board_t *board, piece_t piece, square_t square)
{
    const bitboard_t    bitsquare = square_bit(square);

    board->table[square] = piece;
    board->piecetype_bits[ALL_PIECES] |= bitsquare;
    board->piecetype_bits[type_of_piece(piece)] |= bitsquare;
    board->color_bits[color_of_piece(piece)] |= bitsquare;
    board->piece_count[piece]++;
    board->piece_count[create_piece(color_of_piece(piece), ALL_PIECES)]++;
    board->psq_scorepair += PsqScore[piece][square];
}

INLINED void        move_piece(board_t *board, square_t from, square_t to)
{
    piece_t        piece = board->table[from];
    bitboard_t    move_bits = square_bit(from) | square_bit(to);

    board->piecetype_bits[ALL_PIECES] ^= move_bits;
    board->piecetype_bits[type_of_piece(piece)] ^= move_bits;
    board->color_bits[color_of_piece(piece)] ^= move_bits;
    board->table[from] = NO_PIECE;
    board->table[to] = piece;
    board->psq_scorepair += PsqScore[piece][to] - PsqScore[piece][from];
}

INLINED void        remove_piece(board_t *board, square_t square)
{
    piece_t                piece = board->table[square];
    const bitboard_t    bitsquare = square_bit(square);

    board->piecetype_bits[ALL_PIECES] ^= bitsquare;
    board->piecetype_bits[type_of_piece(piece)] ^= bitsquare;
    board->color_bits[color_of_piece(piece)] ^= bitsquare;
    board->piece_count[piece]--;
    board->piece_count[create_piece(color_of_piece(piece), ALL_PIECES)]--;
    board->psq_scorepair -= PsqScore[piece][square];
}

INLINED void        do_move(board_t *board, move_t move, boardstack_t *stack)
{
    do_move_gc(board, move, stack, move_gives_check(board, move));
}

#endif
