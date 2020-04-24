/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   generate_move_values.c                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 22:01:59 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:56:34 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"

void	generate_move_values(movelist_t *movelist, const board_t *board,
		move_t tt_move, move_t *killers)
{
	for (size_t i = 0; i < movelist_size(movelist); ++i)
	{
		move_t		move = movelist->moves[i].move;

		if (move == tt_move)
		{
			movelist->moves[i].score = 8192;
			continue ;
		}

		square_t	from = move_from_square(move);
		square_t	to = move_to_square(move);

		piece_t		moved_piece = piece_on(board, from);
		piece_t		captured_piece = piece_on(board, to);

		if (type_of_move(move) == PROMOTION)
			movelist->moves[i].score = 4096 + PieceScores[ENDGAME][promotion_type(move)];

		else if (type_of_move(move) == CASTLING)
			movelist->moves[i].score = 512;

		else if (type_of_move(move) == EN_PASSANT)
			movelist->moves[i].score = 2048 + PAWN * 8 - PAWN;

		else if (captured_piece != NO_PIECE)
		{
			movelist->moves[i].score = see_greater_than(board, move, 0) ? 2048 : 1024;

			movelist->moves[i].score += type_of_piece(captured_piece) * 8
				- type_of_piece(moved_piece);
		}
		else if (killers && (move == killers[0] || move == killers[1]))
			movelist->moves[i].score = 1536;
		else
		{
			movelist->moves[i].score =
				midgame_score(PsqScore[moved_piece][to])
				- midgame_score(PsqScore[moved_piece][from]);

			if (board->side_to_move == BLACK)
				movelist->moves[i].score = -movelist->moves[i].score;
		}
	}
}
