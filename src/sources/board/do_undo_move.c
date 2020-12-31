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

#include <assert.h>
#include "board.h"
#include "imath.h"
#include "lazy_smp.h"
#include "tt.h"

void    do_move_gc(board_t *board, move_t move, boardstack_t *next,
        bool gives_check)
{
    get_worker(board)->nodes += 1;

    hashkey_t   key = board->stack->board_key ^ ZobristBlackToMove;

    next->castlings = board->stack->castlings;
    next->rule50 = board->stack->rule50;
    next->plies_from_null_move = board->stack->plies_from_null_move;
    next->en_passant_square = board->stack->en_passant_square;
    next->pawn_key = board->stack->pawn_key;
    next->material[WHITE] = board->stack->material[WHITE];
    next->material[BLACK] = board->stack->material[BLACK];

    next->prev = board->stack;
    board->stack = next;

    board->ply += 1;
    board->stack->rule50 += 1;
    board->stack->plies_from_null_move += 1;

    color_t     us = board->side_to_move;
    color_t     them = not_color(us);
    square_t    from = move_from_square(move);
    square_t    to = move_to_square(move);
    piece_t     piece = piece_on(board, from);
    piece_t     captured_piece = type_of_move(move) == EN_PASSANT
        ? create_piece(them, PAWN) : piece_on(board, to);

    if (type_of_move(move) == CASTLING)
    {
        square_t    rook_from;
        square_t    rook_to;

        do_castling(board, us, from, &to, &rook_from, &rook_to);

        key ^= ZobristPsq[captured_piece][rook_from];
        key ^= ZobristPsq[captured_piece][rook_to];

        captured_piece = NO_PIECE;
    }

    if (captured_piece)
    {
        square_t    captured_square = to;

        if (type_of_piece(captured_piece) == PAWN)
        {
            if (type_of_move(move) == EN_PASSANT)
                captured_square -= pawn_direction(us);

            board->stack->pawn_key ^= ZobristPsq[captured_piece][captured_square];
        }
        else
            board->stack->material[them] -= PieceScores[MIDGAME][type_of_piece(captured_piece)];

        remove_piece(board, captured_square);

        if (type_of_move(move) == EN_PASSANT)
            board->table[captured_square] = NO_PIECE;

        key ^= ZobristPsq[captured_piece][captured_square];
        board->stack->rule50 = 0;
    }

    key ^= ZobristPsq[piece][from];
    key ^= ZobristPsq[piece][to];

    if (board->stack->en_passant_square != SQ_NONE)
    {
        key ^= ZobristEnPassant[file_of_square(board->stack->en_passant_square)];
        board->stack->en_passant_square = SQ_NONE;
    }

    if (board->stack->castlings && (board->castling_mask[from]
        | board->castling_mask[to]))
    {
        int castling = board->castling_mask[from] | board->castling_mask[to];
        key ^= ZobristCastling[board->stack->castlings & castling];
        board->stack->castlings &= ~castling;
    }

    if (type_of_move(move) != CASTLING)
        move_piece(board, from, to);

    if (type_of_piece(piece) == PAWN)
    {
        if ((to ^ from) == 16 && (pawn_moves(to - pawn_direction(us), us)
            & board->piecetype_bits[PAWN] & board->color_bits[them]))
        {
            board->stack->en_passant_square = to - pawn_direction(us);
            key ^= ZobristEnPassant[file_of_square(board->stack->en_passant_square)];
        }
        else if (type_of_move(move) == PROMOTION)
        {
            piece_t new_piece = create_piece(us, promotion_type(move));

            remove_piece(board, to);
            put_piece(board, new_piece, to);

            key ^= ZobristPsq[piece][to] ^ ZobristPsq[new_piece][to];
            board->stack->pawn_key ^= ZobristPsq[piece][to];
            board->stack->material[us] += PieceScores[MIDGAME][promotion_type(move)];
        }

        board->stack->pawn_key ^= ZobristPsq[piece][from]
            ^ ZobristPsq[piece][to];

        board->stack->rule50 = 0;
    }

    board->stack->captured_piece = captured_piece;
    board->stack->board_key = key;

    prefetch(tt_entry_at(key));

    board->stack->checkers = gives_check ? attackers_to(board,
        board_king_square(board, them)) & board->color_bits[us]
        : 0;

    board->side_to_move = not_color(board->side_to_move);

    set_check(board, board->stack);

    board->stack->repetition = 0;

    int repetition_plies = min(board->stack->rule50, board->stack->plies_from_null_move);

    if (repetition_plies >= 4)
    {
        boardstack_t    *rewind = board->stack->prev->prev;
        for (int i = 4; i <= repetition_plies; i += 2)
        {
            rewind = rewind->prev->prev;
            if (rewind->board_key == board->stack->board_key)
            {
                board->stack->repetition = rewind->repetition ? -i : i;
                break ;
            }
        }
    }
}

void    undo_move(board_t *board, move_t move)
{
    board->side_to_move = not_color(board->side_to_move);

    color_t     us = board->side_to_move;
    square_t    from = move_from_square(move);
    square_t    to = move_to_square(move);
    piece_t     piece = piece_on(board, to);

    if (type_of_move(move) == PROMOTION)
    {
        remove_piece(board, to);
        piece = create_piece(us, PAWN);
        put_piece(board, piece, to);
    }

    if (type_of_move(move) == CASTLING)
    {
        square_t    rook_from;
        square_t    rook_to;
        undo_castling(board, us, from, &to, &rook_from, &rook_to);
    }
    else
    {
        move_piece(board, to, from);

        if (board->stack->captured_piece)
        {
            square_t    capture_square = to;

            if (type_of_move(move) == EN_PASSANT)
                capture_square -= pawn_direction(us);

            put_piece(board, board->stack->captured_piece, capture_square);
        }
    }

    board->stack = board->stack->prev;
    board->ply -= 1;
}
