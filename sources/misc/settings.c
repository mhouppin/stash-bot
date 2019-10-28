/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   settings.c                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:11:42 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/28 16:00:55 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "settings.h"

int				g_debug = DEBUG_OFF;
pthread_mutex_t	mtx_debug = PTHREAD_MUTEX_INITIALIZER;

int				g_threads = 1;
size_t			g_hash = 16ul * 1048576ul;
int				g_multipv = 1;
clock_t			g_mintime = 20ul * 1000ul / CLOCKS_PER_SEC;
