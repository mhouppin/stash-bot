/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 20:20:19 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/04/01 13:15:15 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef ENGINE_H
# define ENGINE_H

# include <time.h>
# include "board.h"

typedef struct
{
	int		plies;
	move_t	*pv;
}
searchstack_t;

void	engine_go(void);
void	search_bestmove(board_t *board, int depth, size_t pv_line,
		move_t *display_pv);

bool	out_of_time(void);
score_t	evaluate(const board_t *board);

#endif