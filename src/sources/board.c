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

#include "board.h"
#include "movelist.h"
#include "tt.h"
#include "types.h"
#include "uci.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char PieceIndexes[PIECE_NB] = " PNBRQK  pnbrqk";

hashkey_t CyclicKeys[8192];
move_t CyclicMoves[8192];

INLINED uint16_t cyclic_index_lo(hashkey_t key) { return key & 0x1FFFu; }

INLINED uint16_t cyclic_index_hi(hashkey_t key) { return (key >> 13) & 0x1FFFu; }

void cyclic_init(void)
{
    // Map all reversible move Zobrist keys to their corresponding move.
    for (piecetype_t pt = KNIGHT; pt <= KING; ++pt)
        for (color_t c = WHITE; c <= BLACK; ++c)
            for (square_t from = SQ_A1; from <= SQ_H8; ++from)
                for (square_t to = from + 1; to <= SQ_H8; ++to)
                    if (piece_moves(pt, from, 0) & square_bb(to))
                    {
                        move_t move = create_move(from, to);
                        const piece_t piece = create_piece(c, pt);
                        hashkey_t key =
                            ZobristPsq[piece][from] ^ ZobristPsq[piece][to] ^ ZobristBlackToMove;

                        uint16_t index = cyclic_index_lo(key);

                        // Swap the current move/key pair with the table content
                        // until we find an empty slot.
                        while (true)
                        {
                            hashkey_t tmpKey = CyclicKeys[index];
                            CyclicKeys[index] = key;
                            key = tmpKey;

                            move_t tmpMove = CyclicMoves[index];
                            CyclicMoves[index] = move;
                            move = tmpMove;

                            if (move == NO_MOVE) break;

                            // Trick: change the section of the key for indexing
                            // by xor-ing the index value with the low and high
                            // key indexes:
                            // - if index == hi, index ^ lo ^ hi == lo
                            // - if index == lo, index ^ lo ^ hi == hi
                            index ^= cyclic_index_lo(key) ^ cyclic_index_hi(key);
                        }
                    }
}

static bool board_invalid_material(const Board *board, color_t c)
{
    int pawns = board->pieceCount[create_piece(c, PAWN)];
    const int knights = board->pieceCount[create_piece(c, KNIGHT)];
    const int bishops = board->pieceCount[create_piece(c, BISHOP)];
    const int rooks = board->pieceCount[create_piece(c, ROOK)];
    const int queens = board->pieceCount[create_piece(c, QUEEN)];

    // Compute the number of pieces that are guaranteed to be promoted Pawns.
    const int pknights = imax(0, knights - 2);
    const int pbishops = imax(0, bishops - 2);
    const int prooks = imax(0, rooks - 2);
    const int pqueens = imax(0, queens - 1);
    const int promoted = pknights + pbishops + prooks + pqueens;

    pawns += promoted;

    return (pawns > 8) || (knights + pawns - pknights > 10) || (bishops + pawns - pbishops > 10)
           || (rooks + pawns - prooks > 10) || (queens + pawns - pqueens > 9);
}

static int board_parse_fen_pieces(Board *board, const char *fen)
{
    char *ptr;
    square_t square = SQ_A8;
    rank_t curRank = RANK_8;
    size_t nextSection = strcspn(fen, Delimiters);

    for (size_t i = 0; i < nextSection; ++i)
    {
        if (fen[i] >= '1' && fen[i] <= '8')
        {
            square += fen[i] - '0';

            // Check that we don't have more than 8 squares for a given rank.
            if (sq_rank(square - 1) > curRank)
            {
                debug_printf(
                    "info error Invalid FEN: too much squares on a single rank in piece section\n");
                return -1;
            }
        }
        else if (fen[i] == '/')
        {
            if (square != create_sq(FILE_A, curRank + 1))
            {
                debug_printf("info error Invalid FEN: not enough squares on a single rank in piece "
                             "section\n");
                return -1;
            }

            if (curRank == RANK_1)
            {
                debug_printf("info error Invalid FEN: too much ranks in piece section\n");
                return -1;
            }

            square += 2 * SOUTH;
            --curRank;
        }
        else if (ptr = strchr(PieceIndexes, fen[i]), ptr != NULL)
        {
            // Check that we don't have more than 8 squares for a given rank.
            if (sq_rank(square) > curRank)
            {
                debug_printf(
                    "info error Invalid FEN: too much squares on a single rank in piece section\n");
                return -1;
            }

            put_piece(board, (piece_t)(ptr - (const char *)PieceIndexes), square++);
        }
        else
        {
            debug_printf("info error Invalid FEN: invalid character in piece section\n");
            return -1;
        }
    }

    // Check if the final number of ranks is correct.
    if (curRank != RANK_1)
    {
        debug_printf("info error Invalid FEN: missing ranks in piece section\n");
        return -1;
    }

    // Check if the final number of squares is correct.
    if (sq_file(square) != FILE_A)
    {
        debug_printf(
            "info error Invalid FEN: not enough squares on a single rank in piece section\n");
        return -1;
    }

    // Check if each side has a unique King on the board.
    if (board->pieceCount[WHITE_KING] != 1 || board->pieceCount[BLACK_KING] != 1)
    {
        debug_printf("info error Invalid FEN: invalid number of Kings on the board\n");
        return -1;
    }

    // Check if the Kings aren't next to each other.
    if (SquareDistance[get_king_square(board, WHITE)][get_king_square(board, BLACK)] == 1)
    {
        debug_printf("info error Invalid FEN: Kings touching each other\n");
        return -1;
    }

    // Check that there are no pawns on the first and last ranks.
    if (piecetype_bb(board, PAWN) & (RANK_1_BB | RANK_8_BB))
    {
        debug_printf("info error Invalid FEN: Pawns on first/last ranks\n");
        return -1;
    }

    // Since we have no guarantees on correct behavior for the move generator
    // with more pieces than there are normally in a game, check that the
    // material configuration on the board can exist.
    if (board_invalid_material(board, WHITE) || board_invalid_material(board, BLACK))
    {
        debug_printf("info error Invalid FEN: invalid material distribution on the board\n");
        return -1;
    }

    // Return the number of bytes parsed.
    return (int)nextSection;
}

static int board_parse_stm(Board *board, const char *fen)
{
    const size_t nextSection = strcspn(fen, Delimiters);

    // Check that the STM field only contains at most a single character.
    if (nextSection > 1)
    {
        debug_printf("info error Invalid FEN: too many characters for side to move section\n");
        return -1;
    }

    // Check that, if present, the STM field is either 'w' or 'b'.
    if (*fen == 'b')
        board->sideToMove = BLACK;

    else if (*fen == 'w' || *fen == '\0')
        board->sideToMove = WHITE;

    else
    {
        debug_printf("info error Invalid FEN: invalid character in side to move section\n");
        return -1;
    }

    // Return the number of bytes parsed.
    return (int)nextSection;
}

static int board_set_castling(Board *board, color_t color, square_t rookSquare)
{
    const square_t kingSquare = get_king_square(board, color);
    const int castling = (color == WHITE ? WHITE_CASTLING : BLACK_CASTLING)
                         & (kingSquare < rookSquare ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

    // Check that the King is on the first rank relative to its color.
    if (relative_sq_rank(kingSquare, color) != RANK_1)
    {
        debug_printf(
            "info error Invalid FEN: castling rights given with the king not on back-rank\n");
        return -1;
    }

    // If the King or the Rook are not on their usual squares, we're playing
    // Chess960.
    if (sq_file(kingSquare) != FILE_E
        || (sq_file(rookSquare) != FILE_A && sq_file(rookSquare) != FILE_H))
        board->chess960 = true;

    board->stack->castlings |= castling;
    board->castlingMask[kingSquare] |= castling;
    board->castlingMask[rookSquare] |= castling;
    board->castlingRookSquare[castling] = rookSquare;

    const square_t kingAfter = relative_sq((castling & KINGSIDE_CASTLING) ? SQ_G1 : SQ_C1, color);
    const square_t rookAfter = relative_sq((castling & KINGSIDE_CASTLING) ? SQ_F1 : SQ_D1, color);

    board->castlingPath[castling] =
        (between_bb(rookSquare, rookAfter) | between_bb(kingSquare, kingAfter)
            | square_bb(rookAfter) | square_bb(kingAfter))
        & ~(square_bb(kingSquare) | square_bb(rookSquare));

    return 0;
}

static int board_parse_castling(Board *board, const char *fen)
{
    size_t nextSection = strcspn(fen, Delimiters);

    for (size_t i = 0; i < nextSection; ++i)
    {
        // Check that if '-' is specified, it is the only character of the section.
        if (fen[i] == '-')
        {
            if (nextSection > 1)
            {
                debug_printf("info error Invalid FEN: '-' specified in castling section with extra "
                             "characters\n");
                return -1;
            }

            break;
        }

        square_t rookSquare;
        const color_t side = islower((unsigned char)fen[i]) ? BLACK : WHITE;
        const piece_t rook = create_piece(side, ROOK);
        const char castlingChar = toupper((unsigned char)fen[i]);

        // Compute the rook square based on the parsed character.
        if (castlingChar == 'K')
        {
            for (rookSquare = relative_sq(SQ_H1, side); sq_file(rookSquare) != FILE_A; --rookSquare)
                if (piece_on(board, rookSquare) == rook) break;
        }
        else if (castlingChar == 'Q')
        {
            for (rookSquare = relative_sq(SQ_A1, side); sq_file(rookSquare) != FILE_H; ++rookSquare)
                if (piece_on(board, rookSquare) == rook) break;
        }
        else if (castlingChar >= 'A' && castlingChar <= 'H')
            rookSquare = create_sq(castlingChar - 'A', relative_rank(RANK_1, side));

        else
        {
            debug_printf("info error Invalid FEN: invalid character in castling section\n");
            return -1;
        }

        // Set the castling rights while checking that they are valid.
        if (board_set_castling(board, side, rookSquare) < 0) return -1;
    }

    return (int)nextSection;
}

static int board_parse_en_passant(Board *board, const char *fen)
{
    size_t nextSection = strcspn(fen, Delimiters);

    board->stack->enPassantSquare = SQ_NONE;

    // Check that if there's a single character in the section, it's a '-'.
    if (nextSection == 1 && *fen != '-')
    {
        debug_printf("info error Invalid FEN: invalid character in en-passant section\n");
        return -1;
    }
    // Check that there's no more than 2 characters in the section.
    else if (nextSection > 2)
    {
        debug_printf("info error Invalid FEN: too many characters in en-passant section\n");
        return -1;
    }
    // Stop the parsing if the field isn't present or contains a '-'.
    else if (nextSection <= 1)
        return nextSection;

    const char fileChar = fen[0];
    const char rankChar = fen[1];

    // Check that the en-passant square is correctly formatted and valid for the side to move.
    if (fileChar < 'a' || fileChar > 'h' || rankChar != (board->sideToMove == WHITE ? '6' : '3'))
    {
        debug_printf("info error Invalid FEN: invalid or ill-formed en-passant square\n");
        return -1;
    }

    const square_t epSquare = create_sq(fileChar - 'a', rankChar - '1');

    const color_t us = board->sideToMove;
    const color_t them = not_color(us);

    // Check that there is an opponent's Pawn under the en-passant square.
    if (!(piece_bb(board, them, PAWN) & square_bb(epSquare + pawn_direction(them))))
    {
        debug_printf("info error Invalid FEN: en-passant square set with no pawn under it\n");
        return -1;
    }

    // Set the en-passant square only if at least one of our pawns is in
    // position of performing the move in a pseudo-legal way.
    if (attackers_to(board, epSquare) & piece_bb(board, us, PAWN))
        board->stack->enPassantSquare = epSquare;

    return (int)nextSection;
}

int board_from_fen(Board *board, const char *fen, bool isChess960, Boardstack *bstack)
{
    memset(board, 0, sizeof(Board));
    memset(bstack, 0, sizeof(Boardstack));
    int result;

    board->stack = bstack;

    // Skip any trailing whitespaces.
    fen += strspn(fen, Delimiters);

    // Start by parsing the piece section.
    result = board_parse_fen_pieces(board, fen);

    if (result < 0) return result;

    // From here, if the remaining sections are missing, we just assume default
    // values for them.
    fen += result;
    fen += strspn(fen, Delimiters);

    // Parse the side to move section.
    result = board_parse_stm(board, fen);

    if (result < 0) return result;

    fen += result;
    fen += strspn(fen, Delimiters);

    // Check that the King of the opposite side isn't in check.
    if (attackers_to(board, get_king_square(board, not_color(board->sideToMove)))
        & color_bb(board, board->sideToMove))
    {
        debug_printf("info error Invalid FEN: opposite King is in check\n");
        return -1;
    }

    // Parse the castling section.
    result = board_parse_castling(board, fen);

    if (result < 0) return result;

    fen += result;
    fen += strspn(fen, Delimiters);

    // Delayed verification: if the UCI_Chess960 option isn't set while we
    // receive Chess960-styled castling rights, issue a warning in debug mode.
    if (!isChess960 && board->chess960)
        debug_printf(
            "info string Warning: FRC position emitted with the UCI_Chess960 flag unset\n");

    // Force the Chess960 flag to be set for standard positions with
    // UCI_Chess960 enabled.
    board->chess960 = isChess960;

    // Parse the en-passant square section.
    result = board_parse_en_passant(board, fen);

    if (result < 0) return result;

    fen += result;
    fen += strspn(fen, Delimiters);

    // Parse the rule50 section. Accepted values are restrained to the [-1024, 1024] range.
    {
        const size_t nextSection = strcspn(fen, Delimiters);
        char *ptr;
        const long rule50l = strtol(fen, &ptr, 10);

        // Check for ill-formed rule50 fields or values outside the accepted range.
        if (fen + nextSection != ptr || rule50l < -1024 || rule50l > 1024)
        {
            debug_printf("info error Invalid FEN: invalid rule50 field\n");
            return -1;
        }

        fen += nextSection;
        board->stack->rule50 = rule50l;
    }

    fen += strspn(fen, Delimiters);

    // Parse the move number section. Accepted values are restrained to the [0, 2048] range,
    // with 0 being converted to 1.
    {
        const size_t nextSection = strcspn(fen, Delimiters);
        char *ptr;
        const long moveNumber = strtol(fen, &ptr, 10);

        // Check for ill-formed move number fields or values outside the accepted range.
        if (fen + nextSection != ptr || moveNumber < 0 || moveNumber > 2048)
        {
            debug_printf("info error Invalid FEN: invalid move number field\n");
            return -1;
        }

        fen += nextSection;
        board->ply = moveNumber;
        board->ply = imax(0, 2 * (board->ply - 1));
        board->ply += (board->sideToMove == BLACK);
    }

    // Initialize the stack data.
    set_boardstack(board, board->stack);

    // Check that there are not more than 2 pieces giving check to the side to
    // move, as it cannot happen in a normal chess game and may cause issues
    // during move generation.
    if (popcount(board->stack->checkers) > 2)
    {
        debug_printf("info error Invalid FEN: more than 2 pieces giving check to the King\n");
        return -1;
    }

    return 0;
}

void set_boardstack(Board *board, Boardstack *stack)
{
    stack->boardKey = stack->pawnKey = board->stack->materialKey = 0;
    stack->material[WHITE] = stack->material[BLACK] = 0;
    stack->checkers = attackers_to(board, get_king_square(board, board->sideToMove))
                      & color_bb(board, not_color(board->sideToMove));

    set_check(board, stack);

    // Compute the board key, pawn key, and material distribution values from
    // the occupancy bitboards.
    for (bitboard_t b = occupancy_bb(board); b;)
    {
        const square_t square = bb_pop_first_sq(&b);
        const piece_t piece = piece_on(board, square);

        stack->boardKey ^= ZobristPsq[piece][square];

        if (piece_type(piece) == PAWN)
            stack->pawnKey ^= ZobristPsq[piece][square];

        else if (piece_type(piece) != KING)
            stack->material[piece_color(piece)] += PieceScores[MIDGAME][piece];
    }

    // And the en-passant square to the board key if it exists.
    if (stack->enPassantSquare != SQ_NONE)
        stack->boardKey ^= ZobristEnPassant[sq_file(stack->enPassantSquare)];

    if (board->sideToMove == BLACK) stack->boardKey ^= ZobristBlackToMove;

    // Compute the material key used for indexing specialized endgames.
    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt)
        {
            const piece_t pc = create_piece(c, pt);

            for (int i = 0; i < board->pieceCount[pc]; ++i) stack->materialKey ^= ZobristPsq[pc][i];
        }

    // And the castlings to the board key.
    stack->boardKey ^= ZobristCastling[stack->castlings];
}

void set_check(Board *restrict board, Boardstack *restrict stack)
{
    // Store the list of pieces that are currently pinned to their King.
    stack->kingBlockers[WHITE] = slider_blockers(
        board, color_bb(board, BLACK), get_king_square(board, WHITE), &stack->pinners[BLACK]);
    stack->kingBlockers[BLACK] = slider_blockers(
        board, color_bb(board, WHITE), get_king_square(board, BLACK), &stack->pinners[WHITE]);

    const square_t kingSquare = get_king_square(board, not_color(board->sideToMove));

    // Compute the list of squares from which the side to move can give check
    // to the opponent's King.
    stack->checkSquares[PAWN] = pawn_moves(kingSquare, not_color(board->sideToMove));
    stack->checkSquares[KNIGHT] = knight_moves(kingSquare);
    stack->checkSquares[BISHOP] = bishop_moves(board, kingSquare);
    stack->checkSquares[ROOK] = rook_moves(board, kingSquare);
    stack->checkSquares[QUEEN] = stack->checkSquares[BISHOP] | stack->checkSquares[ROOK];
    stack->checkSquares[KING] = 0;
}

Boardstack *dup_boardstack(const Boardstack *stack)
{
    if (!stack) return NULL;

    Boardstack *const newStack = malloc(sizeof(Boardstack));

    // Recursively copy the list of sub-stacks.
    *newStack = *stack;
    newStack->prev = dup_boardstack(stack->prev);
    return newStack;
}

void free_boardstack(Boardstack *stack)
{
    while (stack)
    {
        Boardstack *const next = stack->prev;
        free(stack);
        stack = next;
    }
}

const char *board_fen(const Board *board)
{
    const char *pieceToChar = " PNBRQK  pnbrqk";
    static char fenBuffer[128];
    char *ptr = fenBuffer;

    // Start by generating the piece section of the FEN.
    for (rank_t rank = RANK_8; rank >= RANK_1; --rank)
    {
        for (file_t file = FILE_A; file <= FILE_H; ++file)
        {
            int emptyCount;

            // Span through all consecutive empty squares on the rank.
            for (emptyCount = 0; file <= FILE_H && empty_square(board, create_sq(file, rank));
                 ++file)
                ++emptyCount;

            // Place a digit corresponding to the number of consecutive empty
            // squares if needed.
            if (emptyCount) *(ptr++) = emptyCount + '0';

            // Then write the next piece present on the rank if it exists.
            if (file <= FILE_H) *(ptr++) = pieceToChar[piece_on(board, create_sq(file, rank))];
        }

        if (rank > RANK_1) *(ptr++) = '/';
    }

    // Add the side to move.
    *(ptr++) = ' ';
    *(ptr++) = (board->sideToMove == WHITE) ? 'w' : 'b';
    *(ptr++) = ' ';

    // Add castling rights, either with "KQkq" characters or with "A-Ha-h" if
    // we're playing fischerandom.
    if (board->stack->castlings & WHITE_OO)
        *(ptr++) = board->chess960 ? 'A' + sq_file(board->castlingRookSquare[WHITE_OO]) : 'K';

    if (board->stack->castlings & WHITE_OOO)
        *(ptr++) = board->chess960 ? 'A' + sq_file(board->castlingRookSquare[WHITE_OOO]) : 'Q';

    if (board->stack->castlings & BLACK_OO)
        *(ptr++) = board->chess960 ? 'a' + sq_file(board->castlingRookSquare[BLACK_OO]) : 'k';

    if (board->stack->castlings & BLACK_OOO)
        *(ptr++) = board->chess960 ? 'a' + sq_file(board->castlingRookSquare[BLACK_OOO]) : 'q';

    // Write a '-' if no castling is available.
    if (!(board->stack->castlings & ANY_CASTLING)) *(ptr++) = '-';

    *(ptr++) = ' ';

    // Write the en-passant square if it exists.
    if (board->stack->enPassantSquare == SQ_NONE)
        *(ptr++) = '-';
    else
    {
        *(ptr++) = 'a' + sq_file(board->stack->enPassantSquare);
        *(ptr++) = '1' + sq_rank(board->stack->enPassantSquare);
    }

    // Finally, write the rule50 counter value and the move number.
    sprintf(
        ptr, " %d %d", board->stack->rule50, 1 + (board->ply - (board->sideToMove == BLACK)) / 2);

    return fenBuffer;
}

void do_move_gc(Board *restrict board, move_t move, Boardstack *restrict next, bool givesCheck)
{
    hashkey_t key = board->stack->boardKey ^ ZobristBlackToMove;

    // Copy the state variables that will need to be updated incrementally.
    // Don't copy things like the checking squares, since they need to be
    // computed from scratch after each move.
    next->castlings = board->stack->castlings;
    next->rule50 = board->stack->rule50;
    next->pliesFromNullMove = board->stack->pliesFromNullMove;
    next->enPassantSquare = board->stack->enPassantSquare;
    next->pawnKey = board->stack->pawnKey;
    next->materialKey = board->stack->materialKey;
    next->material[WHITE] = board->stack->material[WHITE];
    next->material[BLACK] = board->stack->material[BLACK];

    // Link the new stack to the existing list, and increment the ply-related
    // counters.
    next->prev = board->stack;
    board->stack = next;
    board->ply += 1;
    board->stack->rule50 += 1;
    board->stack->pliesFromNullMove += 1;

    const color_t us = board->sideToMove, them = not_color(us);
    square_t from = from_sq(move), to = to_sq(move);
    const piece_t piece = piece_on(board, from);
    piece_t capturedPiece =
        move_type(move) == EN_PASSANT ? create_piece(them, PAWN) : piece_on(board, to);

    // Special handling for castling rights, since we encode them as "King
    // captures Rook".
    if (move_type(move) == CASTLING)
    {
        square_t rookFrom, rookTo;

        do_castling(board, us, from, &to, &rookFrom, &rookTo);

        key ^= ZobristPsq[capturedPiece][rookFrom];
        key ^= ZobristPsq[capturedPiece][rookTo];

        capturedPiece = NO_PIECE;
    }

    // Additional work is required for captures.
    if (capturedPiece)
    {
        square_t capturedSquare = to;

        if (piece_type(capturedPiece) == PAWN)
        {
            if (move_type(move) == EN_PASSANT) capturedSquare -= pawn_direction(us);

            board->stack->pawnKey ^= ZobristPsq[capturedPiece][capturedSquare];
        }
        else
            board->stack->material[them] -= PieceScores[MIDGAME][capturedPiece];

        remove_piece(board, capturedSquare);

        // Special handling for en-passant moves: since the arrival square of
        // the Pawn isn't the one the opponent's Pawn resides on, we need to
        // remove it manually.
        if (move_type(move) == EN_PASSANT) board->table[capturedSquare] = NO_PIECE;

        // Update the board and material keys, and reset the rule50 counter.
        key ^= ZobristPsq[capturedPiece][capturedSquare];
        board->stack->materialKey ^= ZobristPsq[capturedPiece][board->pieceCount[capturedPiece]];
        board->stack->rule50 = 0;
    }

    key ^= ZobristPsq[piece][from];
    key ^= ZobristPsq[piece][to];

    // Clear the en-passant Zobrist key from the board key if needed.
    if (board->stack->enPassantSquare != SQ_NONE)
    {
        key ^= ZobristEnPassant[sq_file(board->stack->enPassantSquare)];
        board->stack->enPassantSquare = SQ_NONE;
    }

    // If our move modified castling rights, update the board key and the list
    // of available castlings.
    if (board->stack->castlings && (board->castlingMask[from] | board->castlingMask[to]))
    {
        const int castling = board->castlingMask[from] | board->castlingMask[to];
        key ^= ZobristCastling[board->stack->castlings & castling];
        board->stack->castlings &= ~castling;
    }

    // Don't move the piece during castling, since we already do this work in
    // the do_castling() function.
    if (move_type(move) != CASTLING) move_piece(board, from, to);

    // Additional work is required for pawn moves.
    if (piece_type(piece) == PAWN)
    {
        // Set the en-passant square if we moved our Pawn two squares and one of
        // the opponent's Pawns is next to our Pawn.
        if ((to ^ from) == 16
            && (pawn_moves(to - pawn_direction(us), us) & piece_bb(board, them, PAWN)))
        {
            board->stack->enPassantSquare = to - pawn_direction(us);
            key ^= ZobristEnPassant[sq_file(board->stack->enPassantSquare)];
        }
        // Additional work is required for promotion.
        else if (move_type(move) == PROMOTION)
        {
            const piece_t newPiece = create_piece(us, promotion_type(move));

            remove_piece(board, to);
            put_piece(board, newPiece, to);

            // Update the board, Pawn and material keys.
            key ^= ZobristPsq[piece][to] ^ ZobristPsq[newPiece][to];
            board->stack->pawnKey ^= ZobristPsq[piece][to];
            board->stack->material[us] += PieceScores[MIDGAME][promotion_type(move)];
            board->stack->materialKey ^= ZobristPsq[newPiece][board->pieceCount[newPiece] - 1];
            board->stack->materialKey ^= ZobristPsq[piece][board->pieceCount[piece]];
        }

        // Update the Pawn key and clear the rule50 counter.
        board->stack->pawnKey ^= ZobristPsq[piece][from] ^ ZobristPsq[piece][to];
        board->stack->rule50 = 0;
    }

    // Record the captured piece if it exists, and set the board key.
    board->stack->capturedPiece = capturedPiece;
    board->stack->boardKey = key;

    // Prefetch the TT entry as early as possible.
    prefetch(tt_entry_at(key));

    // Save the list of checking pieces if the move gives check.
    board->stack->checkers =
        givesCheck ? attackers_to(board, get_king_square(board, them)) & color_bb(board, us) : 0;

    // Swap the side to move.
    board->sideToMove = not_color(board->sideToMove);
    set_check(board, board->stack);

    // Compute the repetition counter for the reached position.
    board->stack->repetition = 0;

    const int repetitionPlies = imin(board->stack->rule50, board->stack->pliesFromNullMove);

    // Only check for 2-fold or 3-fold repetitions when the last irreversible
    // move is at least 4 plies away.
    if (repetitionPlies >= 4)
    {
        Boardstack *rewind = board->stack->prev->prev;
        for (int i = 4; i <= repetitionPlies; i += 2)
        {
            rewind = rewind->prev->prev;
            if (rewind->boardKey == board->stack->boardKey)
            {
                board->stack->repetition = rewind->repetition ? -i : i;
                break;
            }
        }
    }
}

void undo_move(Board *board, move_t move)
{
    // Swap the side to move.
    board->sideToMove = not_color(board->sideToMove);

    const color_t us = board->sideToMove;
    square_t from = from_sq(move), to = to_sq(move);

    // If the move was a promotion, place the Pawn back on its square.
    if (move_type(move) == PROMOTION)
    {
        remove_piece(board, to);
        put_piece(board, create_piece(us, PAWN), to);
    }

    // Special handling is required for castling moves.
    if (move_type(move) == CASTLING)
    {
        square_t rookFrom, rookTo;
        undo_castling(board, us, from, &to, &rookFrom, &rookTo);
    }
    else
    {
        move_piece(board, to, from);

        // If we captured a piece, place it back on the board.
        if (board->stack->capturedPiece)
        {
            square_t captureSquare = to;

            if (move_type(move) == EN_PASSANT) captureSquare -= pawn_direction(us);

            put_piece(board, board->stack->capturedPiece, captureSquare);
        }
    }

    // Unlink the last stack, and decrement the ply counter.
    board->stack = board->stack->prev;
    board->ply -= 1;
}

void do_castling(Board *restrict board, color_t us, square_t kingFrom, square_t *restrict kingTo,
    square_t *restrict rookFrom, square_t *restrict rookTo)
{
    // Get the castling side based on whether the Rook is on the left or the
    // right side of the King.
    const bool kingside = *kingTo > kingFrom;

    // Since we encode castling moves as "King takes Rook", the Rook initial
    // position corresponds to the arrival square of the move. We then
    // compute the real arrival squares of the King and Rook based on the
    // castling side.
    *rookFrom = *kingTo;
    *rookTo = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *kingTo = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    // Update the King and Rook positions on the board.
    remove_piece(board, kingFrom);
    remove_piece(board, *rookFrom);
    board->table[kingFrom] = board->table[*rookFrom] = NO_PIECE;
    put_piece(board, create_piece(us, KING), *kingTo);
    put_piece(board, create_piece(us, ROOK), *rookTo);
}

void undo_castling(Board *restrict board, color_t us, square_t kingFrom, square_t *restrict kingTo,
    square_t *restrict rookFrom, square_t *restrict rookTo)
{
    // Get the castling side based on whether the Rook is on the left or the
    // right side of the King.
    bool kingside = *kingTo > kingFrom;

    // Since we encode castling moves as "King takes Rook", the Rook initial
    // position corresponds to the arrival square of the move. We then
    // compute the real arrival squares of the King and Rook based on the
    // castling side.
    *rookFrom = *kingTo;
    *rookTo = relative_sq(kingside ? SQ_F1 : SQ_D1, us);
    *kingTo = relative_sq(kingside ? SQ_G1 : SQ_C1, us);

    // Update the King and Rook positions on the board.
    remove_piece(board, *kingTo);
    remove_piece(board, *rookTo);
    board->table[*kingTo] = board->table[*rookTo] = NO_PIECE;
    put_piece(board, create_piece(us, KING), kingFrom);
    put_piece(board, create_piece(us, ROOK), *rookFrom);
}

void do_null_move(Board *restrict board, Boardstack *restrict stack)
{
    // Copy the whole stack state. (NOTE: this part might be further optimized
    // by only copying the required fields as in the do_move_gc() function, this
    // hasn't been tested yet for an Elo gain.)
    memcpy(stack, board->stack, sizeof(Boardstack));

    // Link the new stack to the existing list.
    stack->prev = board->stack;
    board->stack = stack;

    // Clear the en-passant Zobrist key from the board key if needed.
    if (stack->enPassantSquare != SQ_NONE)
    {
        stack->boardKey ^= ZobristEnPassant[sq_file(stack->enPassantSquare)];
        stack->enPassantSquare = SQ_NONE;
    }

    stack->boardKey ^= ZobristBlackToMove;

    // Prefetch the TT entry as early as possible.
    prefetch(tt_entry_at(stack->boardKey));

    // Increment the rule50 counter, and reset the pliesFromNullMove one.
    ++stack->rule50;
    stack->pliesFromNullMove = 0;

    // Swtp the side to move.
    board->sideToMove = not_color(board->sideToMove);
    set_check(board, stack);

    // We don't check for repetition during null moves, since they are
    // artificially inserted into the game state and don't correspond to actual
    // available moves.
    stack->repetition = 0;
}

void undo_null_move(Board *board)
{
    // Simply unlink the last stack and swap the side to move.
    board->stack = board->stack->prev;
    board->sideToMove = not_color(board->sideToMove);
}

bitboard_t attackers_list(const Board *board, square_t s, bitboard_t occupied)
{
    return ((pawn_moves(s, BLACK) & piece_bb(board, WHITE, PAWN))
            | (pawn_moves(s, WHITE) & piece_bb(board, BLACK, PAWN))
            | (knight_moves(s) & piecetype_bb(board, KNIGHT))
            | (rook_moves_bb(s, occupied) & piecetypes_bb(board, ROOK, QUEEN))
            | (bishop_moves_bb(s, occupied) & piecetypes_bb(board, BISHOP, QUEEN))
            | (king_moves(s) & piecetype_bb(board, KING)));
}

bitboard_t slider_blockers(
    const Board *restrict board, bitboard_t sliders, square_t square, bitboard_t *restrict pinners)
{
    bitboard_t blockers = *pinners = 0;
    bitboard_t snipers = ((PseudoMoves[ROOK][square] & piecetypes_bb(board, QUEEN, ROOK))
                             | (PseudoMoves[BISHOP][square] & piecetypes_bb(board, QUEEN, BISHOP)))
                         & sliders;
    const bitboard_t occupied = occupancy_bb(board) ^ snipers;

    while (snipers)
    {
        const square_t sniperSquare = bb_pop_first_sq(&snipers);
        const bitboard_t between = between_bb(square, sniperSquare) & occupied;

        // There are no pins if there are two or more pieces between the slider
        // piece and the target square.
        if (between && !more_than_one(between))
        {
            blockers |= between;
            if (between & color_bb(board, piece_color(piece_on(board, square))))
                *pinners |= square_bb(sniperSquare);
        }
    }

    return blockers;
}

bool game_is_drawn(const Board *board, int ply)
{
    // Check if we have a draw by 50-move rule.
    if (board->stack->rule50 > 99)
    {
        // If the side to move isn't in check, we can claim a draw.
        if (!board->stack->checkers) return true;

        Movelist movelist;

        // If we have at least one legal move, we can claim a draw.
        list_all(&movelist, board);
        if (movelist_size(&movelist) != 0) return true;
    }

    // If we have a repetition that comes from the search state (and not from
    // the given UCI position history), we can claim a draw. This means that
    // we check for 2-fold repetitions coming from solely the search root
    // position and later, and for 3-fold if there are older 2-fold repetitions
    // in the position history.
    return !!board->stack->repetition && board->stack->repetition < ply;
}

bool game_has_cycle(const Board *board, int ply)
{
    const int maxPlies = imin(board->stack->rule50, board->stack->pliesFromNullMove);

    // If we have less than 3 plies without an irreversible move or a null move,
    // we are guaranteed to not find a cycle in the position.
    if (maxPlies < 3) return false;

    const hashkey_t originalKey = board->stack->boardKey;
    const Boardstack *stackIt = board->stack->prev;

    for (int i = 3; i <= maxPlies; i += 2)
    {
        // Only check for cycles from a single side.
        stackIt = stackIt->prev->prev;

        const hashkey_t moveKey = originalKey ^ stackIt->boardKey;

        // Check if the move key corresponds to a reversible move.
        uint16_t index = cyclic_index_lo(moveKey);
        if (CyclicKeys[index] != moveKey)
        {
            index = cyclic_index_hi(moveKey);
            if (CyclicKeys[index] != moveKey) continue;
        }

        const move_t move = CyclicMoves[index];
        const square_t from = from_sq(move);
        const square_t to = to_sq(move);

        // Check if there are no pieces between the 'from' and 'to' square.
        if (between_bb(from, to) & occupancy_bb(board)) continue;

        // If the cycle is contained in the search tree, return true.
        if (ply > i) return true;

        // For nodes before the search tree or at the root, we check if the
        // move created a repetition earlier (in the same logic spirit that
        // we check for 2-fold repetitions in the search tree, or 3-fold for
        // repetitions prior to root).
        if (!stackIt->repetition) continue;

        // Verify that we're making the cycle with our move. (This is only
        // necessary for repetitions prior to root).
        if (piece_color(piece_on(board, empty_square(board, from) ? to : from))
            != board->sideToMove)
            return true;
    }

    return false;
}

bool move_gives_check(const Board *board, move_t move)
{
    const square_t from = from_sq(move), to = to_sq(move);
    square_t captureSquare;
    square_t kingFrom, rookFrom, kingTo, rookTo;
    bitboard_t occupied;
    const color_t us = board->sideToMove, them = not_color(board->sideToMove);

    // Test if the move is a direct check to the King.
    if (board->stack->checkSquares[piece_type(piece_on(board, from))] & square_bb(to)) return true;

    const square_t theirKing = get_king_square(board, them);

    // Test if the move is a discovered check.
    if ((board->stack->kingBlockers[them] & square_bb(from)) && !sq_aligned(from, to, theirKing))
        return true;

    // We need special handling for "non-standard moves".
    switch (move_type(move))
    {
        case NORMAL_MOVE: return false;

        // In the case of a promotion, test if the promoted piece gives a direct
        // check to the King from the arrival square.
        case PROMOTION:
            return (piece_moves(promotion_type(move), to, occupancy_bb(board) ^ square_bb(from))
                    & square_bb(theirKing));

        // In the case of en-passant captures, test if either the captured Pawn
        // or the moving Pawn are uncovering an attack on the King.
        case EN_PASSANT:
            captureSquare = create_sq(sq_file(to), sq_rank(from));
            occupied =
                (occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSquare)) | square_bb(to);

            return (rook_moves_bb(theirKing, occupied) & pieces_bb(board, us, QUEEN, ROOK))
                   | (bishop_moves_bb(theirKing, occupied) & pieces_bb(board, us, QUEEN, BISHOP));

        // In the case of a castling, test if the Rook gives a direct check from
        // its arrival square.
        case CASTLING:
            kingFrom = from;
            rookFrom = to;
            kingTo = relative_sq(rookFrom > kingFrom ? SQ_G1 : SQ_C1, us);
            rookTo = relative_sq(rookFrom > kingFrom ? SQ_F1 : SQ_D1, us);

            return (PseudoMoves[ROOK][rookTo] & square_bb(theirKing))
                   && (rook_moves_bb(
                           rookTo, (occupancy_bb(board) ^ square_bb(kingFrom) ^ square_bb(rookFrom))
                                       | square_bb(kingTo) | square_bb(rookTo))
                       & square_bb(theirKing));

        default: __builtin_unreachable(); return false;
    }
}

bool move_is_legal(const Board *board, move_t move)
{
    const color_t us = board->sideToMove;
    square_t from = from_sq(move), to = to_sq(move);

    // Special handling for en-passant captures.
    if (move_type(move) == EN_PASSANT)
    {
        // Check for any discovered check on our King with the capture.
        const square_t kingSquare = get_king_square(board, us);
        const square_t captureSquare = to - pawn_direction(us);
        const bitboard_t occupied =
            (occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSquare)) | square_bb(to);

        return !(rook_moves_bb(kingSquare, occupied) & pieces_bb(board, not_color(us), QUEEN, ROOK))
               && !(bishop_moves_bb(kingSquare, occupied)
                    & pieces_bb(board, not_color(us), QUEEN, BISHOP));
    }

    // Special handling for castlings.
    if (move_type(move) == CASTLING)
    {
        // Check for any opponent's piece attack along the King path.
        to = relative_sq((to > from ? SQ_G1 : SQ_C1), us);

        direction_t side = (to > from ? WEST : EAST);

        for (square_t sq = to; sq != from; sq += side)
            if (attackers_to(board, sq) & color_bb(board, not_color(us))) return false;

        return !board->chess960
               || !(rook_moves_bb(to, occupancy_bb(board) ^ square_bb(to_sq(move)))
                    & pieces_bb(board, not_color(us), ROOK, QUEEN));
    }

    // If the King is moving, check for any opponent's piece attack on the
    // arrival square.
    if (piece_type(piece_on(board, from)) == KING)
        return (!(attackers_to(board, to) & color_bb(board, not_color(us))));

    // If the moving piece is pinned, check if the move generates a discovered
    // check.
    return !(board->stack->kingBlockers[us] & square_bb(from))
           || sq_aligned(from, to, get_king_square(board, us));
}

bool move_is_pseudo_legal(const Board *board, move_t move)
{
    const color_t us = board->sideToMove;
    const square_t from = from_sq(move), to = to_sq(move);
    const piece_t piece = piece_on(board, from);

    // We perform a slower, but simpler check for "non-standard" moves.
    if (move_type(move) != NORMAL_MOVE)
    {
        Movelist list;

        list_pseudo(&list, board);
        return movelist_has_move(&list, move);
    }

    // The move is normal, so the promotion type bits cannot be set.
    // (Note that this check will likely never trigger, since recent CPU archs
    // guarantee atomic reads/writes to aligned memory.)
    if (promotion_type(move) != KNIGHT) return false;

    // Check if there is a piece that belongs to us on the 'from' square.
    if (piece == NO_PIECE || piece_color(piece) != us) return false;

    // Check if the arrival square doesn't contain one of our pieces.
    // (Note that even though castling is encoded as 'King takes Rook', it is
    // handled in the "uncommon case" scope above.)
    if (color_bb(board, us) & square_bb(to)) return false;

    if (piece_type(piece) == PAWN)
    {
        // The Pawn cannot arrive on a promotion square, since the move is not
        // flagged as a promotion.
        if ((RANK_8_BB | RANK_1_BB) & square_bb(to)) return false;

        // Check if the Pawn move is a valid capture, push, or double push.
        if (!(pawn_moves(from, us) & color_bb(board, not_color(us)) & square_bb(to))
            && !((from + pawn_direction(us) == to) && empty_square(board, to))
            && !((from + 2 * pawn_direction(us) == to) && relative_sq_rank(from, us) == RANK_2
                 && empty_square(board, to) && empty_square(board, to - pawn_direction(us))))
            return false;
    }

    // Check if the piece can reach the arrival square from its position.
    else if (!(piece_moves(piece_type(piece), from, occupancy_bb(board)) & square_bb(to)))
        return false;

    if (board->stack->checkers)
    {
        if (piece_type(piece) != KING)
        {
            // If the side to move is in double check, only a King move can be
            // legal.
            if (more_than_one(board->stack->checkers)) return false;

            // We must either capture the checking piece, or block the attack
            // if it's a slider piece.
            if (!((between_bb(bb_first_sq(board->stack->checkers), get_king_square(board, us))
                      | board->stack->checkers)
                    & square_bb(to)))
                return false;
        }

        // Check if the King is still under the fire of the opponent's pieces
        // after moving.
        else if (attackers_list(board, to, occupancy_bb(board) ^ square_bb(from))
                 & color_bb(board, not_color(us)))
            return false;
    }

    return true;
}

score_t SEE_SCORES[PIECETYPE_NB] = {
    0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE, ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0
};

bool see_greater_than(const Board *board, move_t m, score_t threshold)
{
    // "Non-standard" moves are tricky to evaluate, so perform a generic check
    // here.
    if (move_type(m) != NORMAL_MOVE) return threshold <= 0;

    const square_t from = from_sq(m), to = to_sq(m);
    score_t nextScore = SEE_SCORES[piece_type(piece_on(board, to))] - threshold;

    // If we can't get enough material with the sole capture of the piece, stop.
    if (nextScore < 0) return false;

    nextScore = SEE_SCORES[piece_type(piece_on(board, from))] - nextScore;

    // If our opponent cannot get enough material back by capturing our moved
    // piece, stop.
    if (nextScore <= 0) return true;

    bitboard_t occupied = occupancy_bb(board) ^ square_bb(from) ^ square_bb(to);
    color_t sideToMove = piece_color(piece_on(board, from));
    bitboard_t attackers = attackers_list(board, to, occupied);
    bitboard_t stmAttackers, b;
    int result = 1;

    // Perform exchanges on the target square, until one side has no attackers
    // left or fails to pass the material threshold with its next capture.
    while (true)
    {
        sideToMove = not_color(sideToMove);
        attackers &= occupied;

        // Stop the loop if one of the sides has no attackers left.
        if (!(stmAttackers = attackers & color_bb(board, sideToMove))) break;

        // Exclude pinned pieces from the list of available attackers.
        if (board->stack->pinners[not_color(sideToMove)] & occupied)
        {
            stmAttackers &= ~board->stack->kingBlockers[sideToMove];
            if (!stmAttackers) break;
        }

        result ^= 1;

        // Use the pieces for capturing in the ascending order of their material
        // values. Update the attackers' list as pieces get removed to include
        // X-ray attacks due to Bishops/Rooks/Queens being lined up on the
        // target square.
        if ((b = stmAttackers & piecetype_bb(board, PAWN)))
        {
            if ((nextScore = SEE_SCORES[PAWN] - nextScore) < result) break;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, KNIGHT)))
        {
            if ((nextScore = SEE_SCORES[KNIGHT] - nextScore) < result) break;

            occupied ^= square_bb(bb_first_sq(b));
        }
        else if ((b = stmAttackers & piecetype_bb(board, BISHOP)))
        {
            if ((nextScore = SEE_SCORES[BISHOP] - nextScore) < result) break;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, ROOK)))
        {
            if ((nextScore = SEE_SCORES[ROOK] - nextScore) < result) break;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else if ((b = stmAttackers & piecetype_bb(board, QUEEN)))
        {
            if ((nextScore = SEE_SCORES[QUEEN] - nextScore) < result) break;

            occupied ^= square_bb(bb_first_sq(b));
            attackers |= bishop_moves_bb(to, occupied) & piecetypes_bb(board, BISHOP, QUEEN);
            attackers |= rook_moves_bb(to, occupied) & piecetypes_bb(board, ROOK, QUEEN);
        }
        else
            return ((attackers & ~color_bb(board, sideToMove)) ? result ^ 1 : result);
    }
    return result;
}
