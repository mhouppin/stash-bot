/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   movelist.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 16:02:56 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 22:02:17 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef MOVELIST_H
# define MOVELIST_H

# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include "board.h"
# include "inlining.h"
# include "move.h"

typedef struct	extmove_s
{
	move_t	move;
	score_t	score;
}				extmove_t;

typedef struct	movelist_s
{
	extmove_t	moves[256];
	extmove_t	*last;
}				movelist_t;

extmove_t	*generate_all(extmove_t *movelist, const board_t *board);

extmove_t	*generate_classic(extmove_t *movelist, const board_t *board);
extmove_t	*generate_evasions(extmove_t *movelist, const board_t *board);

extmove_t	*generate_knight_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target_squares);
extmove_t	*generate_bishop_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target_squares);
extmove_t	*generate_rook_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target_squares);
extmove_t	*generate_queen_moves(extmove_t *movelist, const board_t *board,
			color_t us, bitboard_t target_squares);

void		sort_moves(extmove_t *begin, extmove_t *end);
void		generate_move_values(movelist_t *movelist, const board_t *board);

INLINED void	list_all(movelist_t *movelist, const board_t *board)
{
	movelist->last = generate_all(movelist->moves, board);
}

INLINED size_t	movelist_size(const movelist_t *movelist)
{
	return (movelist->last - movelist->moves);
}

INLINED const extmove_t	*movelist_begin(const movelist_t *movelist)
{
	return (movelist->moves);
}

INLINED const extmove_t	*movelist_end(const movelist_t *movelist)
{
	return (movelist->last);
}

INLINED bool	movelist_has_move(const movelist_t *movelist, move_t move)
{
	for (const extmove_t *extmove = movelist_begin(movelist);
			extmove < movelist_end(movelist); ++extmove)
		if (extmove->move == move)
			return (true);

	return (false);
}

#endif
