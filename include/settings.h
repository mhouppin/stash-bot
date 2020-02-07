/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   settings.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:11:00 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/06 14:43:56 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef SETTINGS_H
# define SETTINGS_H

# include <pthread.h>
# include <time.h>
# include <stddef.h>

enum	e_debug
{
	DEBUG_OFF,
	DEBUG_ON
};

extern int				g_debug;

extern int				g_threads;
extern size_t			g_hash;
extern int				g_multipv;
extern clock_t			g_mintime;
extern clock_t			g_overhead;

#endif
