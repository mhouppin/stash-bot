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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "imath.h"
#include "lazy_smp.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

void    set_board(board_t *board, char *fen, bool is_chess960,
        boardstack_t *bstack)
{
    square_t    square = SQ_A8;
    char        *ptr = fen;
    char        *fen_pieces = get_next_token(&ptr);
    char        *fen_side_to_move = get_next_token(&ptr);
    char        *fen_castlings = get_next_token(&ptr);
    char        *fen_en_passant = get_next_token(&ptr);
    char        *fen_rule50 = get_next_token(&ptr);
    char        *fen_turn = get_next_token(&ptr);

    memset(board, 0, sizeof(board_t));
    memset(bstack, 0, sizeof(boardstack_t));

    board->stack = bstack;

    // Scans the piece section of the FEN.

    for (size_t i = 0; fen_pieces[i]; ++i)
    {
        switch (fen_pieces[i])
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                square += (fen_pieces[i] - '0');
                break ;

            case '/':
                square += 2 * SOUTH;
                break ;

            case 'P':
                put_piece(board, WHITE_PAWN, square++);
                break ;

            case 'N':
                put_piece(board, WHITE_KNIGHT, square++);
                break ;

            case 'B':
                put_piece(board, WHITE_BISHOP, square++);
                break ;

            case 'R':
                put_piece(board, WHITE_ROOK, square++);
                break ;

            case 'Q':
                put_piece(board, WHITE_QUEEN, square++);
                break ;

            case 'K':
                put_piece(board, WHITE_KING, square++);
                break ;

            case 'p':
                put_piece(board, BLACK_PAWN, square++);
                break ;

            case 'n':
                put_piece(board, BLACK_KNIGHT, square++);
                break ;

            case 'b':
                put_piece(board, BLACK_BISHOP, square++);
                break ;

            case 'r':
                put_piece(board, BLACK_ROOK, square++);
                break ;

            case 'q':
                put_piece(board, BLACK_QUEEN, square++);
                break ;

            case 'k':
                put_piece(board, BLACK_KING, square++);
                break ;
        }
    }

    // Gets the side to move.

    board->side_to_move = (strcmp(fen_side_to_move, "w") == 0 ? WHITE : BLACK);

    // Gets the allowed castlings.

    for (size_t i = 0; fen_castlings[i]; ++i)
    {
        square_t    rook_square;
        color_t     color = islower(fen_castlings[i]) ? BLACK : WHITE;
        piece_t     rook = create_piece(color, ROOK);

        fen_castlings[i] = toupper(fen_castlings[i]);

        if (fen_castlings[i] == 'K')
            for (rook_square = relative_sq(SQ_H1, color);
                piece_on(board, rook_square) != rook; --rook_square) {}

        else if (fen_castlings[i] == 'Q')
            for (rook_square = relative_sq(SQ_A1, color);
                piece_on(board, rook_square) != rook; ++rook_square) {}

        else if (fen_castlings[i] >= 'A' && fen_castlings[i] <= 'H')
            rook_square = create_sq((file_t)(fen_castlings[i] - 'A'),
                relative_rank(RANK_1, color));

        else
            continue ;

        set_castling(board, color, rook_square);
    }

    // Gets the en-passant square, if any.

    if (fen_en_passant[0] >= 'a' && fen_en_passant[0] <= 'h'
        && (fen_en_passant[1] == '3' || fen_en_passant[1] == '6'))
    {
        board->stack->en_passant_square = create_sq(
            fen_en_passant[0] - 'a', fen_en_passant[1] - '1');

        // If no pawn is able to make the en passant capture,
        // or no opponent pawn is present in front of the en passant square,
        // remove it.

        if (!(attackers_to(board, board->stack->en_passant_square)
                & piece_bb(board, board->side_to_move, PAWN))
            || !(piece_bb(board, not_color(board->side_to_move), PAWN)
                & square_bb(board->stack->en_passant_square
                    + pawn_direction(not_color(board->side_to_move)))))
            board->stack->en_passant_square = SQ_NONE;
    }
    else
        board->stack->en_passant_square = SQ_NONE;

    // If the user omits the last two fields (50mr and plies from start),
    // allow the parsing to still work correctly.

    if (!fen_rule50)
        board->stack->rule50 = 0;
    else
        board->stack->rule50 = atoi(fen_rule50);

    if (!fen_turn)
        board->ply = 0;
    else
        board->ply = atoi(fen_turn);

    board->ply = 2 * (board->ply - 1);

    if (board->ply < 0)
        board->ply = 0;

    board->ply += (board->side_to_move == BLACK);

    board->chess960 = is_chess960;

    set_boardstack(board, board->stack);
}

void    set_boardstack(board_t *board, boardstack_t *stack)
{
    stack->board_key = stack->pawn_key = board->stack->material_key = 0;
    stack->material[WHITE] = stack->material[BLACK] = 0;
    stack->checkers = attackers_to(board, get_king_square(board, board->side_to_move))
        & color_bb(board, not_color(board->side_to_move));

    set_check(board, stack);

    for (bitboard_t b = board->piecetype_bits[ALL_PIECES]; b; )
    {
        square_t    square = bb_pop_first_sq(&b);
        piece_t     piece = piece_on(board, square);

        stack->board_key ^= ZobristPsq[piece][square];

        if (piece_type(piece) == PAWN)
            stack->pawn_key ^= ZobristPsq[piece][square];

        else if (piece_type(piece) != KING)
            stack->material[piece_color(piece)] += PieceScores[MIDGAME][piece];
    }

    if (stack->en_passant_square != SQ_NONE)
        stack->board_key ^= ZobristEnPassant[sq_file(stack->en_passant_square)];

    if (board->side_to_move == BLACK)
        stack->board_key ^= ZobristBlackToMove;

    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt)
        {
            piece_t     pc = create_piece(c, pt);

            for (int i = 0; i < board->piece_count[pc]; ++i)
                stack->material_key ^= ZobristPsq[pc][i];
        }

    stack->board_key ^= ZobristCastling[stack->castlings];
}

void    set_castling(board_t *board, color_t color, square_t rook_square)
{
    square_t    king_square = get_king_square(board, color);
    int         castling = (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING)
        & (king_square < rook_square ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

    board->stack->castlings |= castling;
    board->castling_mask[king_square] |= castling;
    board->castling_mask[rook_square] |= castling;
    board->castling_rook_square[castling] = rook_square;

    square_t    king_after = relative_sq(castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1, color);
    square_t    rook_after = relative_sq(castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1, color);

    board->castling_path[castling] =
        (between_bb(rook_square, rook_after) | between_bb(king_square, king_after)
        | square_bb(rook_after) | square_bb(king_after))
        & ~(square_bb(king_square) | square_bb(rook_square));
}

void    set_check(board_t *board, boardstack_t *stack)
{
    stack->king_blockers[WHITE] = slider_blockers(board,
        color_bb(board, BLACK), get_king_square(board, WHITE),
        &stack->pinners[BLACK]);
    stack->king_blockers[BLACK] = slider_blockers(board,
        color_bb(board, WHITE), get_king_square(board, BLACK),
        &stack->pinners[WHITE]);

    square_t    king_square = get_king_square(board, not_color(board->side_to_move));

    stack->check_squares[PAWN] = pawn_moves(king_square, not_color(board->side_to_move));
    stack->check_squares[KNIGHT] = knight_moves(king_square);
    stack->check_squares[BISHOP] = bishop_moves(board, king_square);
    stack->check_squares[ROOK] = rook_moves(board, king_square);
    stack->check_squares[QUEEN] = stack->check_squares[BISHOP] | stack->check_squares[ROOK];
    stack->check_squares[KING] = 0;
}

boardstack_t    *dup_boardstack(const boardstack_t *stack)
{
    if (!stack)
        return (NULL);

    boardstack_t    *new_stack = malloc(sizeof(boardstack_t));

    *new_stack = *stack;
    new_stack->prev = dup_boardstack(stack->prev);
    return (new_stack);
}

void    free_boardstack(boardstack_t *stack)
{
    while (stack)
    {
        boardstack_t    *next = stack->prev;
        free(stack);
        stack = next;
    }
}

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
    next->material_key = board->stack->material_key;
    next->material[WHITE] = board->stack->material[WHITE];
    next->material[BLACK] = board->stack->material[BLACK];

    next->prev = board->stack;
    board->stack = next;

    board->ply += 1;
    board->stack->rule50 += 1;
    board->stack->plies_from_null_move += 1;

    color_t     us = board->side_to_move;
    color_t     them = not_color(us);
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    piece_t     piece = piece_on(board, from);
    piece_t     captured_piece = move_type(move) == EN_PASSANT
        ? create_piece(them, PAWN) : piece_on(board, to);

    if (move_type(move) == CASTLING)
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

        if (piece_type(captured_piece) == PAWN)
        {
            if (move_type(move) == EN_PASSANT)
                captured_square -= pawn_direction(us);

            board->stack->pawn_key ^= ZobristPsq[captured_piece][captured_square];
        }
        else
            board->stack->material[them] -= PieceScores[MIDGAME][captured_piece];

        remove_piece(board, captured_square);

        if (move_type(move) == EN_PASSANT)
            board->table[captured_square] = NO_PIECE;

        key ^= ZobristPsq[captured_piece][captured_square];
        board->stack->material_key ^= ZobristPsq[captured_piece][board->piece_count[captured_piece]];
        board->stack->rule50 = 0;
    }

    key ^= ZobristPsq[piece][from];
    key ^= ZobristPsq[piece][to];

    if (board->stack->en_passant_square != SQ_NONE)
    {
        key ^= ZobristEnPassant[sq_file(board->stack->en_passant_square)];
        board->stack->en_passant_square = SQ_NONE;
    }

    if (board->stack->castlings && (board->castling_mask[from] | board->castling_mask[to]))
    {
        int castling = board->castling_mask[from] | board->castling_mask[to];
        key ^= ZobristCastling[board->stack->castlings & castling];
        board->stack->castlings &= ~castling;
    }

    if (move_type(move) != CASTLING)
        move_piece(board, from, to);

    if (piece_type(piece) == PAWN)
    {
        if ((to ^ from) == 16 && (pawn_moves(to - pawn_direction(us), us)
            & piece_bb(board, them, PAWN)))
        {
            board->stack->en_passant_square = to - pawn_direction(us);
            key ^= ZobristEnPassant[sq_file(board->stack->en_passant_square)];
        }
        else if (move_type(move) == PROMOTION)
        {
            piece_t new_piece = create_piece(us, promotion_type(move));

            remove_piece(board, to);
            put_piece(board, new_piece, to);

            key ^= ZobristPsq[piece][to] ^ ZobristPsq[new_piece][to];
            board->stack->pawn_key ^= ZobristPsq[piece][to];
            board->stack->material[us] += PieceScores[MIDGAME][promotion_type(move)];
            board->stack->material_key ^= ZobristPsq[new_piece][board->piece_count[new_piece] - 1];
            board->stack->material_key ^= ZobristPsq[piece][board->piece_count[piece]];
        }

        board->stack->pawn_key ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];

        board->stack->rule50 = 0;
    }

    board->stack->captured_piece = captured_piece;
    board->stack->board_key = key;

    prefetch(tt_entry_at(key));

    board->stack->checkers = gives_check
        ? attackers_to(board, get_king_square(board, them)) & color_bb(board, us)
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
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    piece_t     piece = piece_on(board, to);

    if (move_type(move) == PROMOTION)
    {
        remove_piece(board, to);
        piece = create_piece(us, PAWN);
        put_piece(board, piece, to);
    }

    if (move_type(move) == CASTLING)
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

            if (move_type(move) == EN_PASSANT)
                capture_square -= pawn_direction(us);

            put_piece(board, board->stack->captured_piece, capture_square);
        }
    }

    board->stack = board->stack->prev;
    board->ply -= 1;
}

void    do_castling(board_t *board, color_t us, square_t king_from,
        square_t *king_to, square_t *rook_from, square_t *rook_to)
{
    bool    kingside = *king_to > king_from;

    *rook_from = *king_to;
    *rook_to = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *king_to = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, king_from);
    remove_piece(board, *rook_from);
    board->table[king_from] = board->table[*rook_from] = NO_PIECE;
    put_piece(board, create_piece(us, KING), *king_to);
    put_piece(board, create_piece(us, ROOK), *rook_to);
}

void    undo_castling(board_t *board, color_t us, square_t king_from,
        square_t *king_to, square_t *rook_from, square_t *rook_to)
{
    bool    kingside = *king_to > king_from;

    *rook_from = *king_to;
    *rook_to = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *king_to = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, *king_to);
    remove_piece(board, *rook_to);
    board->table[*king_to] = board->table[*rook_to] = NO_PIECE;
    put_piece(board, create_piece(us, KING), king_from);
    put_piece(board, create_piece(us, ROOK), *rook_from);
}

void    do_null_move(board_t *board, boardstack_t *stack)
{
    get_worker(board)->nodes += 1;

    memcpy(stack, board->stack, sizeof(boardstack_t));
    stack->prev = board->stack;
    board->stack = stack;

    if (stack->en_passant_square != SQ_NONE)
    {
        stack->board_key ^= ZobristEnPassant[sq_file(stack->en_passant_square)];
        stack->en_passant_square = SQ_NONE;
    }

    stack->board_key ^= ZobristBlackToMove;
    prefetch(tt_entry_at(stack->board_key));

    ++stack->rule50;
    stack->plies_from_null_move = 0;

    board->side_to_move = not_color(board->side_to_move);

    set_check(board, stack);

    stack->repetition = 0;
}

void    undo_null_move(board_t *board)
{
    board->stack = board->stack->prev;
    board->side_to_move = not_color(board->side_to_move);
}

bitboard_t  attackers_list(const board_t *board, square_t s,
            bitboard_t occupied)
{
    return ((pawn_moves(s, BLACK) & piece_bb(board, WHITE, PAWN))
        | (pawn_moves(s, WHITE) & piece_bb(board, BLACK, PAWN))
        | (knight_moves(s) & piecetype_bb(board, KNIGHT))
        | (rook_moves_bb(s, occupied) & piecetypes_bb(board, ROOK, QUEEN))
        | (bishop_moves_bb(s, occupied) & piecetypes_bb(board, BISHOP, QUEEN))
        | (king_moves(s) & piecetype_bb(board, KING)));
}

bitboard_t  slider_blockers(const board_t *board, bitboard_t sliders,
            square_t square, bitboard_t *pinners)
{
    bitboard_t  blockers = *pinners = 0;
    bitboard_t  snipers =
        ((PseudoMoves[ROOK][square] & piecetypes_bb(board, QUEEN, ROOK))
        | (PseudoMoves[BISHOP][square] & piecetypes_bb(board, QUEEN, BISHOP)))
        & sliders;
    bitboard_t  occupied = occupancy_bb(board) ^ snipers;

    while (snipers)
    {
        square_t    sniper_square = bb_pop_first_sq(&snipers);
        bitboard_t  between = between_bb(square, sniper_square) & occupied;

        if (between && !more_than_one(between))
        {
            blockers |= between;
            if (between & color_bb(board, piece_color(piece_on(board, square))))
                *pinners |= square_bb(sniper_square);
        }
    }

    return (blockers);
}

bool    game_is_drawn(const board_t *board, int ply)
{
    if (board->stack->rule50 > 99)
    {
        if (!board->stack->checkers)
            return (true);

        movelist_t  movelist;

        list_all(&movelist, board);

        if (movelist_size(&movelist) != 0)
            return (true);
    }

    if (board->stack->repetition && board->stack->repetition < ply)
        return (true);

    return (false);
}

bool    move_gives_check(const board_t *board, move_t move)
{
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    square_t    capture_square;
    square_t    king_from, rook_from, king_to, rook_to;
    bitboard_t  occupied;

    if (board->stack->check_squares[piece_type(piece_on(board, from))] & square_bb(to))
        return (true);

    square_t    their_king = get_king_square(board, not_color(board->side_to_move));

    if ((board->stack->king_blockers[not_color(board->side_to_move)] & square_bb(from))
        && !sq_aligned(from, to, their_king))
        return (true);

    switch (move_type(move))
    {
        case NORMAL_MOVE:
            return (false);

        case PROMOTION:
            return (piece_moves(promotion_type(move), to, occupancy_bb(board) ^ square_bb(from))
                & square_bb(their_king));

        case EN_PASSANT:
            capture_square = create_sq(sq_file(to), sq_rank(from));

            occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
                | square_bb(to);

            return ((rook_moves_bb(their_king, occupied)
                & pieces_bb(board, board->side_to_move, QUEEN, ROOK))
                | (bishop_moves_bb(their_king, occupied)
                & pieces_bb(board, board->side_to_move, QUEEN, BISHOP)));

        case CASTLING:
            king_from = from;
            rook_from = to;
            king_to = relative_sq(rook_from > king_from ? SQ_G1 : SQ_C1, board->side_to_move);
            rook_to = relative_sq(rook_from > king_from ? SQ_F1 : SQ_D1, board->side_to_move);

            return ((PseudoMoves[ROOK][rook_to] & square_bb(their_king))
                && (rook_moves_bb(rook_to,
                (occupancy_bb(board) ^ square_bb(king_from) ^ square_bb(rook_from))
                | square_bb(king_to) | square_bb(rook_to)) & square_bb(their_king)));

        default:
            __builtin_unreachable();
            return (false);
    }
}

bool    move_is_legal(const board_t *board, move_t move)
{
    color_t     us = board->side_to_move;
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);

    if (move_type(move) == EN_PASSANT)
    {
        // Checks for any discovered check with the en-passant capture.

        square_t    king_square = get_king_square(board, us);
        square_t    capture_square = to - pawn_direction(us);
        bitboard_t  occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(capture_square))
            | square_bb(to);

        return (!(rook_moves_bb(king_square, occupied)
            & pieces_bb(board, not_color(us), QUEEN, ROOK))
            && !(bishop_moves_bb(king_square, occupied)
            & pieces_bb(board, not_color(us), QUEEN, BISHOP)));
    }

    if (move_type(move) == CASTLING)
    {
        // Checks for any opponent piece attack along the king path.

        to = relative_sq((to > from ? SQ_G1 : SQ_C1), us);

        direction_t side = (to > from ? WEST : EAST);

        for (square_t sq = to; sq != from; sq += side)
            if (attackers_to(board, sq) & color_bb(board, not_color(us)))
                return (false);

        return (!board->chess960
            || !(rook_moves_bb(to, occupancy_bb(board) ^ square_bb(to_sq(move)))
                & pieces_bb(board, not_color(us), ROOK, QUEEN)));
    }

    // Checks for any opponent piece attack on the arrival king square.

    if (piece_type(piece_on(board, from)) == KING)
        return (!(attackers_to(board, to) & color_bb(board, not_color(us))));

    // If the moving piece is pinned, checks if the move generates
    // a discovered check.

    return (!(board->stack->king_blockers[us] & square_bb(from))
        || sq_aligned(from, to, get_king_square(board, us)));
}

bool    move_is_pseudo_legal(const board_t *board, move_t move)
{
    color_t     us = board->side_to_move;
    square_t    from = from_sq(move);
    square_t    to = to_sq(move);
    piece_t     piece = piece_on(board, from);

    // Slower check for uncommon cases.

    if (move_type(move) != NORMAL_MOVE)
    {
        movelist_t  list;

        list_pseudo(&list, board);
        return (movelist_has_move(&list, move));
    }

    // The move is normal, so promotion type bits cannot be set.
    // (Note that this check will likely never trigger, since all recent CPUs
    // guarantee atomic reads/writes to memory.)

    if (promotion_type(move) != KNIGHT)
        return (false);

    // Check if there is a piece that belongs to us on the 'from' square.

    if (piece == NO_PIECE || piece_color(piece) != us)
        return (false);

    // Check if there is the 'to' square doesn't contain a friendly piece.
    // (This works because even though castling is encoded as 'King takes Rook',
    // it is handled in the 'uncommon case' scope.

    if (color_bb(board, us) & square_bb(to))
        return (false);

    if (piece_type(piece) == PAWN)
    {
        // The pawn cannot arrive on a promotion square, since we alerady handled
        // the promotion case.

        if ((RANK_8_BITS | RANK_1_BITS) & square_bb(to))
            return (false);

        // Check if the pawn move is a valid capture, push, or double push.

        if (!(pawn_moves(from, us) & color_bb(board, not_color(us)) & square_bb(to))
            && !((from + pawn_direction(us) == to) && empty_square(board, to))
            && !((from + 2 * pawn_direction(us) == to) && relative_sq_rank(from, us) == RANK_2
                && empty_square(board, to) && empty_square(board, to - pawn_direction(us))))
            return (false);
    }

    // Check if the piece can reach the 'to' square from its position.

    else if (!(piece_moves(piece_type(piece), from, occupancy_bb(board)) & square_bb(to)))
        return (false);

    if (board->stack->checkers)
    {
        if (piece_type(piece) != KING)
        {
            // If double check, only a king move can be legal.

            if (more_than_one(board->stack->checkers))
                return (false);

            // We must either capture the checking piece, or block the attack
            // if it's a slider.

            if (!((between_bb(bb_first_sq(board->stack->checkers), get_king_square(board, us))
                | board->stack->checkers) & square_bb(to)))
                return (false);
        }

        // Check if the King is still under the fire of the opponent's pieces after moving.

        else if (attackers_list(board, to, occupancy_bb(board) ^ square_bb(from))
            & color_bb(board, not_color(us)))
            return (false);
    }

    return (true);
}

bool    see_greater_than(const board_t *board, move_t m, score_t threshold)
{
    if (move_type(m) != NORMAL_MOVE)
        return (threshold <= 0);

    square_t    from = from_sq(m);
    square_t    to = to_sq(m);
    score_t     next_score = PieceScores[MIDGAME][piece_on(board, to)] - threshold;

    if (next_score < 0)
        return (false);

    next_score = PieceScores[MIDGAME][piece_on(board, from)] - next_score;

    if (next_score <= 0)
        return (true);

    bitboard_t  occupied = occupancy_bb(board) ^ square_bb(from) ^ square_bb(to);
    color_t     side_to_move = piece_color(piece_on(board, from));
    bitboard_t  attackers = attackers_list(board, to, occupied);
    bitboard_t  stm_attackers;
    bitboard_t  b;
    int         result = 1;

    while (true)
    {
        side_to_move = not_color(side_to_move);
        attackers &= occupied;

        if (!(stm_attackers = attackers & color_bb(board, side_to_move)))
            break ;

        if (board->stack->pinners[not_color(side_to_move)] & occupied)
        {
            stm_attackers &= ~board->stack->king_blockers[side_to_move];
            if (!stm_attackers)
                break ;
        }

        result ^= 1;

        if ((b = stm_attackers & piecetype_bb(board, PAWN)))
        {
            if ((next_score = PAWN_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stm_attackers & piecetype_bb(board, KNIGHT)))
        {
            if ((next_score = KNIGHT_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
        }
        else if ((b = stm_attackers & piecetype_bb(board, BISHOP)))
        {
            if ((next_score = BISHOP_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stm_attackers & piecetype_bb(board, ROOK)))
        {
            if ((next_score = ROOK_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else if ((b = stm_attackers & piecetype_bb(board, QUEEN)))
        {
            if ((next_score = QUEEN_MG_SCORE - next_score) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else
            return ((attackers & ~color_bb(board, side_to_move)) ? result ^ 1 : result);
    }
    return (result);
}
