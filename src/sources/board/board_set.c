/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   board_set.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/20 08:02:13 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 08:04:18 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

void	board_set(board_t *board, char *fen, bool is_chess960,
		boardstack_t *bstack)
{
	const char	*delim = " \t\n";
	square_t	square = SQ_A8;
	char		*fen_pieces = strtok(fen, delim);
	char		*fen_side_to_move = strtok(NULL, delim);
	char		*fen_castlings = strtok(NULL, delim);
	char		*fen_en_passant = strtok(NULL, delim);
	char		*fen_rule50 = strtok(NULL, delim);
	char		*fen_turn = strtok(NULL, delim);

	memset(board, 0, sizeof(board_t));
	memset(bstack, 0, sizeof(boardstack_t));

	for (piece_t piece = 0; piece < PIECE_NB; ++piece)
		for (int i = 0; i < 16; ++i)
			board->piece_list[piece][i] = SQ_NONE;

	board->stack = bstack;

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

	board->side_to_move = (strcmp(fen_side_to_move, "w") == 0 ? WHITE : BLACK);

	for (size_t i = 0; fen_castlings[i]; ++i)
	{
		square_t	rook_square;
		color_t		color = islower(fen_castlings[i]) ? BLACK : WHITE;
		piece_t		rook = create_piece(color, ROOK);

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
			& board->color_bits[opposite_color(board->side_to_move)]
			& square_bit(board->stack->en_passant_square
			+ pawn_direction(opposite_color(board->side_to_move)))))
			board->stack->en_passant_square = SQ_NONE;
	}
	else
		board->stack->en_passant_square = SQ_NONE;

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
