/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   str_to_move.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 21:20:44 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 23:13:44 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"

move_t	str_to_move(const char *str)
{
	int16_t	from;
	int16_t	to;

	from = ((str[1] - '1') << 3) | (str[0] - 'a');
	to = ((str[3] - '1') << 3) | (str[2] - 'a');
	switch (str[4])
	{
		case 'n':
			return (get_move(from, to) | PROMOTION | TO_KNIGHT);

		case 'b':
			return (get_move(from, to) | PROMOTION | TO_BISHOP);

		case 'r':
			return (get_move(from, to) | PROMOTION | TO_ROOK);

		case 'q':
			return (get_move(from, to) | PROMOTION | TO_QUEEN);

		default:
			return (get_move(from, to));
	}
}
