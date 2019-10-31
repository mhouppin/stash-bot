/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   move_to_str.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/31 01:17:15 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 01:20:57 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "engine.h"
#include <stdlib.h>

char	*move_to_str(move_t move)
{
	char	*ret = malloc(6);

	ret[0] = (move_from(move) & 7) + 'a';
	ret[1] = (move_from(move) >> 3) + '1';
	ret[2] = (move_to(move) & 7) + 'a';
	ret[3] = (move_to(move) >> 3) + '1';

	if (move & PROMOTION)
	{
		switch (move & PROMOTION_MASK)
		{
			case TO_KNIGHT:
				ret[4] = 'n';
				break ;

			case TO_BISHOP:
				ret[4] = 'b';
				break ;

			case TO_ROOK:
				ret[4] = 'r';
				break ;

			case TO_QUEEN:
				ret[4] = 'q';
				break ;
		}
		ret[5] = '\0';
	}
	else
		ret[4] = '\0';

	return (ret);
}
