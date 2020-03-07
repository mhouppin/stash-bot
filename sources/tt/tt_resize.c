/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   tt_resize.c                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/03/05 19:15:57 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/06 11:38:18 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include "tt.h"

transposition_t	g_hashtable = {
	0, NULL, NULL, 0
};

void	tt_resize(size_t mbsize)
{
	extern transposition_t	g_hashtable;

	if (g_hashtable.allocated)
		free(g_hashtable.allocated);

	g_hashtable.cluster_count = mbsize * 1024 * 1024 / sizeof(cluster_t);
	g_hashtable.allocated = malloc(g_hashtable.cluster_count * sizeof(cluster_t));

	if (g_hashtable.allocated == NULL)
	{
		perror("Failed to allocate hashtable");
		exit(EXIT_FAILURE);
	}

	g_hashtable.table = g_hashtable.allocated;

	tt_bzero();
}
