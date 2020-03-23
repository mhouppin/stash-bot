/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   tt_save.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/06 11:29:33 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:39:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "tt.h"

void		tt_save(tt_entry_t *entry, hashkey_t k, score_t s, int d, int b, move_t m)
{
	if (m || k != entry->key)
		entry->bestmove = (uint16_t)m;

	if (k != entry->key || d - DEPTH_OFFSET > entry->depth || b == EXACT_BOUND)
	{
		extern transposition_t	g_hashtable;

		entry->key = k;
		entry->score = s;
		entry->genbound = g_hashtable.generation | (uint8_t)b;
		entry->depth = (d - DEPTH_OFFSET);
	}
}
