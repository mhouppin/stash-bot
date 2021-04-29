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

void set_board(board_t *board, char *fen, bool isChess960, boardstack_t *bstack)
{
    square_t square = SQ_A8;
    char *ptr = fen;
    char *fenPieces = get_next_token(&ptr);
    char *fenSideToMove = get_next_token(&ptr);
    char *fenCastlings = get_next_token(&ptr);
    char *fenEnPassant = get_next_token(&ptr);
    char *fenRule50 = get_next_token(&ptr);
    char *fenTurn = get_next_token(&ptr);

    memset(board, 0, sizeof(board_t));
    memset(bstack, 0, sizeof(boardstack_t));

    board->stack = bstack;

    // Scans the piece section of the FEN.

    for (size_t i = 0; fenPieces[i]; ++i)
    {
        switch (fenPieces[i])
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                square += (fenPieces[i] - '0');
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

    board->sideToMove = (strcmp(fenSideToMove, "w") == 0 ? WHITE : BLACK);

    // Gets the allowed castlings.

    for (size_t i = 0; fenCastlings[i]; ++i)
    {
        square_t rookSquare;
        color_t color = islower(fenCastlings[i]) ? BLACK : WHITE;
        piece_t rook = create_piece(color, ROOK);

        fenCastlings[i] = toupper(fenCastlings[i]);

        if (fenCastlings[i] == 'K')
            for (rookSquare = relative_sq(SQ_H1, color);
                piece_on(board, rookSquare) != rook; --rookSquare) {}

        else if (fenCastlings[i] == 'Q')
            for (rookSquare = relative_sq(SQ_A1, color);
                piece_on(board, rookSquare) != rook; ++rookSquare) {}

        else if (fenCastlings[i] >= 'A' && fenCastlings[i] <= 'H')
            rookSquare = create_sq((file_t)(fenCastlings[i] - 'A'), relative_rank(RANK_1, color));

        else
            continue ;

        set_castling(board, color, rookSquare);
    }

    // Gets the en-passant square, if any.

    if (fenEnPassant[0] >= 'a' && fenEnPassant[0] <= 'h'
        && (fenEnPassant[1] == '3' || fenEnPassant[1] == '6'))
    {
        board->stack->enPassantSquare = create_sq(fenEnPassant[0] - 'a', fenEnPassant[1] - '1');

        // If no pawn is able to make the en passant capture,
        // or no opponent pawn is present in front of the en passant square,
        // remove it.

        color_t us = board->sideToMove, them = not_color(board->sideToMove);
        square_t enPassantSq = board->stack->enPassantSquare;

        if (!(attackers_to(board, enPassantSq) & piece_bb(board, us, PAWN))
            || !(piece_bb(board, them, PAWN) & square_bb(enPassantSq + pawn_direction(them))))
            board->stack->enPassantSquare = SQ_NONE;
    }
    else
        board->stack->enPassantSquare = SQ_NONE;

    // If the user omits the last two fields (50mr and plies from start),
    // allow the parsing to still work correctly.

    board->stack->rule50 = fenRule50 ? atoi(fenRule50) : 0;
    board->ply = fenTurn ? atoi(fenTurn) : 0;
    board->ply = max(0, 2 * (board->ply - 1));
    board->ply += (board->sideToMove == BLACK);
    board->chess960 = isChess960;

    set_boardstack(board, board->stack);
}

void set_boardstack(board_t *board, boardstack_t *stack)
{
    stack->boardKey = stack->pawnKey = board->stack->materialKey = 0;
    stack->material[WHITE] = stack->material[BLACK] = 0;
    stack->checkers = attackers_to(board, get_king_square(board, board->sideToMove))
        & color_bb(board, not_color(board->sideToMove));

    set_check(board, stack);

    for (bitboard_t b = occupancy_bb(board); b; )
    {
        square_t square = bb_pop_first_sq(&b);
        piece_t piece = piece_on(board, square);

        stack->boardKey ^= ZobristPsq[piece][square];

        if (piece_type(piece) == PAWN)
            stack->pawnKey ^= ZobristPsq[piece][square];

        else if (piece_type(piece) != KING)
            stack->material[piece_color(piece)] += PieceScores[MIDGAME][piece];
    }

    if (stack->enPassantSquare != SQ_NONE)
        stack->boardKey ^= ZobristEnPassant[sq_file(stack->enPassantSquare)];

    if (board->sideToMove == BLACK)
        stack->boardKey ^= ZobristBlackToMove;

    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt)
        {
            piece_t pc = create_piece(c, pt);

            for (int i = 0; i < board->pieceCount[pc]; ++i)
                stack->materialKey ^= ZobristPsq[pc][i];
        }

    stack->boardKey ^= ZobristCastling[stack->castlings];
}

void set_castling(board_t *board, color_t color, square_t rookSquare)
{
    square_t kingSquare = get_king_square(board, color);
    int castling = (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING)
        & (kingSquare < rookSquare ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

    board->stack->castlings |= castling;
    board->castlingMask[kingSquare] |= castling;
    board->castlingMask[rookSquare] |= castling;
    board->castlingRookSquare[castling] = rookSquare;

    square_t kingAfter = relative_sq(castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1, color);
    square_t rookAfter = relative_sq(castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1, color);

    board->castlingPath[castling] =
        (between_bb(rookSquare, rookAfter) | between_bb(kingSquare, kingAfter)
        | square_bb(rookAfter) | square_bb(kingAfter))
        & ~(square_bb(kingSquare) | square_bb(rookSquare));
}

void set_check(board_t *board, boardstack_t *stack)
{
    stack->kingBlockers[WHITE] = slider_blockers(board, color_bb(board, BLACK),
        get_king_square(board, WHITE), &stack->pinners[BLACK]);
    stack->kingBlockers[BLACK] = slider_blockers(board, color_bb(board, WHITE),
        get_king_square(board, BLACK), &stack->pinners[WHITE]);

    square_t kingSquare = get_king_square(board, not_color(board->sideToMove));

    stack->checkSquares[PAWN] = pawn_moves(kingSquare, not_color(board->sideToMove));
    stack->checkSquares[KNIGHT] = knight_moves(kingSquare);
    stack->checkSquares[BISHOP] = bishop_moves(board, kingSquare);
    stack->checkSquares[ROOK] = rook_moves(board, kingSquare);
    stack->checkSquares[QUEEN] = stack->checkSquares[BISHOP] | stack->checkSquares[ROOK];
    stack->checkSquares[KING] = 0;
}

boardstack_t *dup_boardstack(const boardstack_t *stack)
{
    if (!stack)
        return (NULL);

    boardstack_t *newStack = malloc(sizeof(boardstack_t));

    *newStack = *stack;
    newStack->prev = dup_boardstack(stack->prev);
    return (newStack);
}

void free_boardstack(boardstack_t *stack)
{
    while (stack)
    {
        boardstack_t *next = stack->prev;
        free(stack);
        stack = next;
    }
}

void do_move_gc(board_t *board, move_t move, boardstack_t *next, bool givesCheck)
{
    get_worker(board)->nodes += 1;

    hashkey_t key = board->stack->boardKey ^ ZobristBlackToMove;

    next->castlings = board->stack->castlings;
    next->rule50 = board->stack->rule50;
    next->pliesFromNullMove = board->stack->pliesFromNullMove;
    next->enPassantSquare = board->stack->enPassantSquare;
    next->pawnKey = board->stack->pawnKey;
    next->materialKey = board->stack->materialKey;
    next->material[WHITE] = board->stack->material[WHITE];
    next->material[BLACK] = board->stack->material[BLACK];

    next->prev = board->stack;
    board->stack = next;
    board->ply += 1;
    board->stack->rule50 += 1;
    board->stack->pliesFromNullMove += 1;

    color_t us = board->sideToMove, them = not_color(us);
    square_t from = from_sq(move), to = to_sq(move);
    piece_t piece = piece_on(board, from);
    piece_t capturedPiece = move_type(move) == EN_PASSANT ? create_piece(them, PAWN) : piece_on(board, to);

    if (move_type(move) == CASTLING)
    {
        square_t rookFrom, rookTo;

        do_castling(board, us, from, &to, &rookFrom, &rookTo);

        key ^= ZobristPsq[capturedPiece][rookFrom];
        key ^= ZobristPsq[capturedPiece][rookTo];

        capturedPiece = NO_PIECE;
    }

    if (capturedPiece)
    {
        square_t capturedSquare = to;

        if (piece_type(capturedPiece) == PAWN)
        {
            if (move_type(move) == EN_PASSANT)
                capturedSquare -= pawn_direction(us);

            board->stack->pawnKey ^= ZobristPsq[capturedPiece][capturedSquare];
        }
        else
            board->stack->material[them] -= PieceScores[MIDGAME][capturedPiece];

        remove_piece(board, capturedSquare);

        if (move_type(move) == EN_PASSANT)
            board->table[capturedSquare] = NO_PIECE;

        key ^= ZobristPsq[capturedPiece][capturedSquare];
        board->stack->materialKey ^= ZobristPsq[capturedPiece][board->pieceCount[capturedPiece]];
        board->stack->rule50 = 0;
    }

    key ^= ZobristPsq[piece][from];
    key ^= ZobristPsq[piece][to];

    if (board->stack->enPassantSquare != SQ_NONE)
    {
        key ^= ZobristEnPassant[sq_file(board->stack->enPassantSquare)];
        board->stack->enPassantSquare = SQ_NONE;
    }

    if (board->stack->castlings && (board->castlingMask[from] | board->castlingMask[to]))
    {
        int castling = board->castlingMask[from] | board->castlingMask[to];
        key ^= ZobristCastling[board->stack->castlings & castling];
        board->stack->castlings &= ~castling;
    }

    if (move_type(move) != CASTLING)
        move_piece(board, from, to);

    if (piece_type(piece) == PAWN)
    {
        if ((to ^ from) == 16 && (pawn_moves(to - pawn_direction(us), us) & piece_bb(board, them, PAWN)))
        {
            board->stack->enPassantSquare = to - pawn_direction(us);
            key ^= ZobristEnPassant[sq_file(board->stack->enPassantSquare)];
        }
        else if (move_type(move) == PROMOTION)
        {
            piece_t newPiece = create_piece(us, promotion_type(move));

            remove_piece(board, to);
            put_piece(board, newPiece, to);

            key ^= ZobristPsq[piece][to] ^ ZobristPsq[newPiece][to];
            board->stack->pawnKey ^= ZobristPsq[piece][to];
            board->stack->material[us] += PieceScores[MIDGAME][promotion_type(move)];
            board->stack->materialKey ^= ZobristPsq[newPiece][board->pieceCount[newPiece] - 1];
            board->stack->materialKey ^= ZobristPsq[piece][board->pieceCount[piece]];
        }

        board->stack->pawnKey ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];

        board->stack->rule50 = 0;
    }

    board->stack->capturedPiece = capturedPiece;
    board->stack->boardKey = key;

    prefetch(tt_entry_at(key));

    board->stack->checkers = givesCheck
        ? attackers_to(board, get_king_square(board, them)) & color_bb(board, us)
        : 0;

    board->sideToMove = not_color(board->sideToMove);

    set_check(board, board->stack);

    board->stack->repetition = 0;

    int repetitionPlies = min(board->stack->rule50, board->stack->pliesFromNullMove);

    if (repetitionPlies >= 4)
    {
        boardstack_t *rewind = board->stack->prev->prev;
        for (int i = 4; i <= repetitionPlies; i += 2)
        {
            rewind = rewind->prev->prev;
            if (rewind->boardKey == board->stack->boardKey)
            {
                board->stack->repetition = rewind->repetition ? -i : i;
                break ;
            }
        }
    }
}

void undo_move(board_t *board, move_t move)
{
    board->sideToMove = not_color(board->sideToMove);

    color_t us = board->sideToMove;
    square_t from = from_sq(move), to = to_sq(move);
    piece_t piece = piece_on(board, to);

    if (move_type(move) == PROMOTION)
    {
        remove_piece(board, to);
        piece = create_piece(us, PAWN);
        put_piece(board, piece, to);
    }

    if (move_type(move) == CASTLING)
    {
        square_t rookFrom, rookTo;
        undo_castling(board, us, from, &to, &rookFrom, &rookTo);
    }
    else
    {
        move_piece(board, to, from);

        if (board->stack->capturedPiece)
        {
            square_t captureSquare = to;

            if (move_type(move) == EN_PASSANT)
                captureSquare -= pawn_direction(us);

            put_piece(board, board->stack->capturedPiece, captureSquare);
        }
    }

    board->stack = board->stack->prev;
    board->ply -= 1;
}

void do_castling(board_t *board, color_t us, square_t kingFrom, square_t *kingTo,
    square_t *rookFrom, square_t *rookTo)
{
    bool kingside = *kingTo > kingFrom;

    *rookFrom = *kingTo;
    *rookTo = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *kingTo = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, kingFrom);
    remove_piece(board, *rookFrom);
    board->table[kingFrom] = board->table[*rookFrom] = NO_PIECE;
    put_piece(board, create_piece(us, KING), *kingTo);
    put_piece(board, create_piece(us, ROOK), *rookTo);
}

void undo_castling(board_t *board, color_t us, square_t kingFrom, square_t *kingTo,
    square_t *rookFrom, square_t *rookTo)
{
    bool kingside = *kingTo > kingFrom;

    *rookFrom = *kingTo;
    *rookTo = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *kingTo = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    remove_piece(board, *kingTo);
    remove_piece(board, *rookTo);
    board->table[*kingTo] = board->table[*rookTo] = NO_PIECE;
    put_piece(board, create_piece(us, KING), kingFrom);
    put_piece(board, create_piece(us, ROOK), *rookFrom);
}

void do_null_move(board_t *board, boardstack_t *stack)
{
    get_worker(board)->nodes += 1;

    memcpy(stack, board->stack, sizeof(boardstack_t));
    stack->prev = board->stack;
    board->stack = stack;

    if (stack->enPassantSquare != SQ_NONE)
    {
        stack->boardKey ^= ZobristEnPassant[sq_file(stack->enPassantSquare)];
        stack->enPassantSquare = SQ_NONE;
    }

    stack->boardKey ^= ZobristBlackToMove;
    prefetch(tt_entry_at(stack->boardKey));

    ++stack->rule50;
    stack->pliesFromNullMove = 0;

    board->sideToMove = not_color(board->sideToMove);

    set_check(board, stack);

    stack->repetition = 0;
}

void undo_null_move(board_t *board)
{
    board->stack = board->stack->prev;
    board->sideToMove = not_color(board->sideToMove);
}

bitboard_t attackers_list(const board_t *board, square_t s, bitboard_t occupied)
{
    return ((pawn_moves(s, BLACK) & piece_bb(board, WHITE, PAWN))
        | (pawn_moves(s, WHITE) & piece_bb(board, BLACK, PAWN))
        | (knight_moves(s) & piecetype_bb(board, KNIGHT))
        | (rook_moves_bb(s, occupied) & piecetypes_bb(board, ROOK, QUEEN))
        | (bishop_moves_bb(s, occupied) & piecetypes_bb(board, BISHOP, QUEEN))
        | (king_moves(s) & piecetype_bb(board, KING)));
}

bitboard_t slider_blockers(const board_t *board, bitboard_t sliders, square_t square, bitboard_t *pinners)
{
    bitboard_t blockers = *pinners = 0;
    bitboard_t snipers =
        ((PseudoMoves[ROOK][square] & piecetypes_bb(board, QUEEN, ROOK))
        | (PseudoMoves[BISHOP][square] & piecetypes_bb(board, QUEEN, BISHOP)))
        & sliders;
    bitboard_t occupied = occupancy_bb(board) ^ snipers;

    while (snipers)
    {
        square_t sniperSquare = bb_pop_first_sq(&snipers);
        bitboard_t between = between_bb(square, sniperSquare) & occupied;

        if (between && !more_than_one(between))
        {
            blockers |= between;
            if (between & color_bb(board, piece_color(piece_on(board, square))))
                *pinners |= square_bb(sniperSquare);
        }
    }

    return (blockers);
}

bool game_is_drawn(const board_t *board, int ply)
{
    if (board->stack->rule50 > 99)
    {
        if (!board->stack->checkers)
            return (true);

        movelist_t movelist;

        list_all(&movelist, board);
        if (movelist_size(&movelist) != 0)
            return (true);
    }

    return (!!board->stack->repetition && board->stack->repetition < ply);
}

bool move_gives_check(const board_t *board, move_t move)
{
    square_t from = from_sq(move), to = to_sq(move);
    square_t captureSquare;
    square_t kingFrom, rookFrom, kingTo, rookTo;
    bitboard_t occupied;
    color_t us = board->sideToMove, them = not_color(board->sideToMove);

    if (board->stack->checkSquares[piece_type(piece_on(board, from))] & square_bb(to))
        return (true);

    square_t theirKing = get_king_square(board, them);

    if ((board->stack->kingBlockers[them] & square_bb(from)) && !sq_aligned(from, to, theirKing))
        return (true);

    switch (move_type(move))
    {
        case NORMAL_MOVE:
            return (false);

        case PROMOTION:
            return (piece_moves(promotion_type(move), to, occupancy_bb(board) ^ square_bb(from))
                & square_bb(theirKing));

        case EN_PASSANT:
            captureSquare = create_sq(sq_file(to), sq_rank(from));
            occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSquare)) | square_bb(to);

            return ((rook_moves_bb(theirKing, occupied) & pieces_bb(board, us, QUEEN, ROOK))
                | (bishop_moves_bb(theirKing, occupied) & pieces_bb(board, us, QUEEN, BISHOP)));

        case CASTLING:
            kingFrom = from;
            rookFrom = to;
            kingTo = relative_sq(rookFrom > kingFrom ? SQ_G1 : SQ_C1, us);
            rookTo = relative_sq(rookFrom > kingFrom ? SQ_F1 : SQ_D1, us);

            return ((PseudoMoves[ROOK][rookTo] & square_bb(theirKing))
                && (rook_moves_bb(rookTo, (occupancy_bb(board) ^ square_bb(kingFrom) ^ square_bb(rookFrom))
                | square_bb(kingTo) | square_bb(rookTo)) & square_bb(theirKing)));

        default:
            __builtin_unreachable();
            return (false);
    }
}

bool move_is_legal(const board_t *board, move_t move)
{
    color_t us = board->sideToMove;
    square_t from = from_sq(move), to = to_sq(move);

    if (move_type(move) == EN_PASSANT)
    {
        // Checks for any discovered check with the en-passant capture.

        square_t kingSquare = get_king_square(board, us);
        square_t captureSquare = to - pawn_direction(us);
        bitboard_t occupied = (occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSquare)) | square_bb(to);

        return (!(rook_moves_bb(kingSquare, occupied) & pieces_bb(board, not_color(us), QUEEN, ROOK))
            && !(bishop_moves_bb(kingSquare, occupied) & pieces_bb(board, not_color(us), QUEEN, BISHOP)));
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

    return (!(board->stack->kingBlockers[us] & square_bb(from)) || sq_aligned(from, to, get_king_square(board, us)));
}

bool move_is_pseudo_legal(const board_t *board, move_t move)
{
    color_t us = board->sideToMove;
    square_t from = from_sq(move), to = to_sq(move);
    piece_t piece = piece_on(board, from);

    // Slower check for uncommon cases.

    if (move_type(move) != NORMAL_MOVE)
    {
        movelist_t list;

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

        else if (attackers_list(board, to, occupancy_bb(board) ^ square_bb(from)) & color_bb(board, not_color(us)))
            return (false);
    }

    return (true);
}

bool see_greater_than(const board_t *board, move_t m, score_t threshold)
{
    if (move_type(m) != NORMAL_MOVE)
        return (threshold <= 0);

    square_t from = from_sq(m), to = to_sq(m);
    score_t nextScore = PieceScores[MIDGAME][piece_on(board, to)] - threshold;

    if (nextScore < 0)
        return (false);

    nextScore = PieceScores[MIDGAME][piece_on(board, from)] - nextScore;

    if (nextScore <= 0)
        return (true);

    bitboard_t occupied = occupancy_bb(board) ^ square_bb(from) ^ square_bb(to);
    color_t sideToMove = piece_color(piece_on(board, from));
    bitboard_t attackers = attackers_list(board, to, occupied);
    bitboard_t stmAttackers, b;
    int result = 1;

    while (true)
    {
        sideToMove = not_color(sideToMove);
        attackers &= occupied;

        if (!(stmAttackers = attackers & color_bb(board, sideToMove)))
            break ;

        if (board->stack->pinners[not_color(sideToMove)] & occupied)
        {
            stmAttackers &= ~board->stack->kingBlockers[sideToMove];
            if (!stmAttackers)
                break ;
        }

        result ^= 1;

        if ((b = stmAttackers & piecetype_bb(board, PAWN)))
        {
            if ((nextScore = PAWN_MG_SCORE - nextScore) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, KNIGHT)))
        {
            if ((nextScore = KNIGHT_MG_SCORE - nextScore) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
        }
        else if ((b = stmAttackers & piecetype_bb(board, BISHOP)))
        {
            if ((nextScore = BISHOP_MG_SCORE - nextScore) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, ROOK)))
        {
            if ((nextScore = ROOK_MG_SCORE - nextScore) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, QUEEN)))
        {
            if ((nextScore = QUEEN_MG_SCORE - nextScore) < result)
                break ;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else
            return ((attackers & ~color_bb(board, sideToMove)) ? result ^ 1 : result);
    }
    return (result);
}
