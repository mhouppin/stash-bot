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
#include "uci.h"

void    board_set(board_t *board, char *fen, bool is_chess960,
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
            for (rook_square = relative_square(SQ_H1, color);
                piece_on(board, rook_square) != rook; --rook_square) {}

        else if (fen_castlings[i] == 'Q')
            for (rook_square = relative_square(SQ_A1, color);
                piece_on(board, rook_square) != rook; ++rook_square) {}

        else if (fen_castlings[i] >= 'A' && fen_castlings[i] <= 'H')
            rook_square = create_square((file_t)(fen_castlings[i] - 'A'),
                relative_rank(RANK_1, color));

        else
            continue ;

        set_castling(board, color, rook_square);
    }

    // Gets the en-passant square, if any.

    if (fen_en_passant[0] >= 'a' && fen_en_passant[0] <= 'h'
        && (fen_en_passant[1] == '3' || fen_en_passant[1] == '6'))
    {
        board->stack->en_passant_square = create_square(
            fen_en_passant[0] - 'a', fen_en_passant[1] - '1');

        // If no pawn is able to make the en passant capture,
        // or no opponent pawn is present in front of the en passant square,
        // remove it.

        if (!(attackers_to(board, board->stack->en_passant_square)
            & board->piecetype_bits[PAWN]
            & board->color_bits[board->side_to_move])
            || !(board->piecetype_bits[PAWN]
            & board->color_bits[not_color(board->side_to_move)]
            & square_bit(board->stack->en_passant_square
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
