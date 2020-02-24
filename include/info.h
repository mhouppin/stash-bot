/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   info.h                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/21 18:19:07 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 21:50:49 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef INFO_H
# define INFO_H

# if defined(_WIN32) || defined(_WIN64)

// Windows has some weird format strings for size_t

#  define SIZE_FORMAT "%I64u"

# else // Assume standard size_t format

#  define SIZE_FORMAT "%zu"

# endif

# include <stdint.h>
# include "board.h"
# include "move.h"
# include "movelist.h"

extern uint64_t		g_nodes;

const char	*move_to_str(move_t move, bool is_chess960);
const char	*score_to_str(score_t score);

move_t		str_to_move(const board_t *board, const char *str);

#endif
