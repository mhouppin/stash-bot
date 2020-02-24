/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   score.c                                          .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 21:53:03 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/23 21:56:24 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include "score.h"

const char	*score_to_str(score_t score)
{
	static char	buf[12];

	if (abs(score) >= MATE_FOUND)
		sprintf(buf, "mate %d", (score > 0 ? MATE - score + 1 : -MATE - score)
			/ 2);
	else
		sprintf(buf, "cp %d", score);

	return (buf);
}
