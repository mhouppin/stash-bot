/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   globals.c                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 13:59:36 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/11/22 17:11:59 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "settings.h"
#include "engine.h"

// Begin mtx_debug
int				g_debug = DEBUG_OFF;
pthread_mutex_t	mtx_debug = PTHREAD_MUTEX_INITIALIZER;
// End mtx_debug

int				g_threads = 1;
size_t			g_hash = 16ul * 1048576ul;
int				g_multipv = 1;
clock_t			g_mintime = 20ul;

// Begin mtx_engine
pthread_mutex_t	mtx_engine = PTHREAD_MUTEX_INITIALIZER;

enum e_egn_mode	g_engine_mode = WAITING;
enum e_egn_send	g_engine_send = DO_NOTHING;
//move_t		*g_searchmoves = NULL;
clock_t			g_wtime = NO_TIME;
clock_t			g_btime = NO_TIME;
clock_t			g_winc = NO_INCREMENT;
clock_t			g_binc = NO_INCREMENT;
int				g_movestogo = NO_MOVESTOGO;
int				g_depth = NO_DEPTH;
int				g_curdepth = NO_DEPTH;
size_t			g_nodes = SIZE_MAX;
_Atomic size_t	g_curnodes = 0;
int				g_mate = NO_MATE;
clock_t			g_start = 0;
clock_t			g_movetime = NO_MOVETIME;
int				g_infinite = NO_INFINITE;

board_t			g_init_board = {{0}, 0, 0, 0, 0, 0};
movelist_t		*g_inter_moves = NULL;
board_t			g_real_board = {{0}, 0, 0, 0, 0, 0};
movelist_t		*g_searchmoves = NULL;
int16_t			*g_valuemoves = NULL;
// End mtx_engine

// Begin mtx_hashtable
pthread_mutex_t	mtx_hashtable = PTHREAD_MUTEX_INITIALIZER;
void			**g_hashtable = NULL;
size_t			g_hash_memory = 0;
size_t			g_hash_size = 0;
// End mtx_hashtable
