/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   tt_probe.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/06 11:21:50 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:27:13 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "tt.h"

tt_entry_t	*tt_probe(hashkey_t key, bool *found)
{
	extern transposition_t	g_hashtable;

	tt_entry_t	*entry = tt_entry_at(key);

	for (int i = 0; i < ClusterSize; ++i)
		if (!entry[i].key || entry[i].key == key)
		{
			entry[i].genbound = (g_hashtable.generation | (entry[i].genbound & 3));
			*found = (bool)entry[i].key;
			return (entry + i);
		}

	tt_entry_t	*replace = entry;

	for (int i = 1; i < ClusterSize; ++i)
		if (replace->depth
				- ((259 + g_hashtable.generation - replace->genbound) & 0xFC)
			> entry[i].depth
				- ((259 + g_hashtable.generation - entry[i].genbound) & 0xFC))
			replace = entry + i;

	*found = false;
	return (replace);
}
