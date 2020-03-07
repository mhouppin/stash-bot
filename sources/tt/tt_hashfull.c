/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   tt_hashfull.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/06 11:27:21 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:29:04 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "tt.h"

int		tt_hashfull(void)
{
	extern transposition_t	g_hashtable;

	int count = 0;

	for (int i = 0; i < 1000; ++i)
		for (int j = 0; j < ClusterSize; ++j)
			count += (g_hashtable.table[i][j].genbound & 0xFC)
				== g_hashtable.generation;

	return (count / ClusterSize);
}
