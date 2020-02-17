/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   settings.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:11:00 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/17 16:18:34 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef SETTINGS_H
# define SETTINGS_H

# include <pthread.h>
# include <time.h>
# include <stddef.h>

extern size_t			g_hash;
extern int				g_multipv;
extern clock_t			g_mintime;
extern clock_t			g_overhead;

#endif
