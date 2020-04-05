/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   tt.h                                             .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/03 16:50:13 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:40:04 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef TT_H
# define TT_H

# include <string.h>
# include "score.h"
# include "hashkey.h"
# include "move.h"

typedef struct
{
	hashkey_t	key;
	score_t		score;
	uint8_t		depth;
	uint8_t		genbound;
	uint16_t	bestmove;
	uint16_t	padding;
}		tt_entry_t;

enum
{
	ClusterSize = 4
};

typedef tt_entry_t	cluster_t[ClusterSize];

typedef struct
{
	size_t		cluster_count;
	cluster_t	*table;
	void		*allocated;
	uint8_t		generation;
}		transposition_t;

INLINED tt_entry_t	*tt_entry_at(hashkey_t k)
{
	extern transposition_t	g_hashtable;

	return (g_hashtable.table[((uint32_t)k * (uint64_t)g_hashtable.cluster_count) >> 32]);
}

INLINED void		tt_clear(void)
{
	extern transposition_t	g_hashtable;

	g_hashtable.generation += 4;
}

INLINED void		tt_bzero(void)
{
	extern transposition_t	g_hashtable;

	memset(g_hashtable.allocated, 0, sizeof(cluster_t) * g_hashtable.cluster_count);
}

INLINED score_t		score_to_tt(score_t s, int plies)
{
	return (s >= MATE_FOUND ? s + plies : s <= -MATE_FOUND ? s - plies : s);
}

INLINED score_t		score_from_tt(score_t s, int plies)
{
	return (s >= MATE_FOUND ? s - plies : s <= -MATE_FOUND ? s + plies : s);
}

tt_entry_t	*tt_probe(hashkey_t key, bool *found);
void		tt_save(tt_entry_t *entry, hashkey_t k, score_t s, int d, int b, move_t m);
int			tt_hashfull(void);
void		tt_resize(size_t mbsize);

#endif
