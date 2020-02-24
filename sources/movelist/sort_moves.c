/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   sort_moves.c                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 21:56:34 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 22:00:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "movelist.h"
#include <unistd.h>

void	sort_moves(extmove_t *begin, extmove_t *end)
{
	const size_t	size = (size_t)(end - begin);

	for (size_t gap = size / 2; gap > 0; gap /= 2)
		for (size_t start = gap; start < size; ++start)
			for (ssize_t i = (ssize_t)(start - gap); i >= 0; i -= gap)
			{
				if (begin[i + gap].score <= begin[i].score)
					break ;
				else
				{
					extmove_t	tmp = begin[i + gap];
					begin[i + gap] = begin[i];
					begin[i] = tmp;
				}
			}
}
