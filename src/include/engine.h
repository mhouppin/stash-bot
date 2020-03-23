/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 20:20:19 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 22:43:03 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef ENGINE_H
# define ENGINE_H

# include <time.h>
# include "board.h"

void	engine_go(void);
void	search_bestmove(board_t *board, int depth, size_t pv_line,
		clock_t start);

score_t	evaluate(const board_t *board);

#endif
