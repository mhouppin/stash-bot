/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   engine.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/30 12:51:36 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2020/02/17 08:32:14 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef ENGINE_H
# define ENGINE_H

# include <stddef.h>
# include <pthread.h>
# include <time.h>
# include <stdint.h>
# include <stdatomic.h>

enum	e_egn_mode
{
	WAITING,
	THINKING
};

enum	e_egn_send
{
	DO_NOTHING,
	DO_THINK,
	DO_EXIT,
	DO_ABORT
};

enum	e_egn_default
{
	NO_TIME = 0,
	NO_INCREMENT = 0,
	NO_MOVESTOGO = 0,
	NO_DEPTH = 255,
	NO_MATE = 0,
	NO_MOVETIME = 0,
	NO_INFINITE = 0,
	OK_INFINITE = 1
};

typedef struct	board_s
{
	int8_t	table[64];
	int		special_moves;
	int		last_active_move;
	int		player;
	int		mscore;
	int		escore;
	int		pcount;
	int		evaluation;
}				board_t;

enum	e_moves
{
	MOVE_NONE,
	SQ_A1 = 0,	SQ_B1,	SQ_C1,	SQ_D1,	SQ_E1,	SQ_F1,	SQ_G1,	SQ_H1,
	SQ_A2,	SQ_B2,	SQ_C2,	SQ_D2,	SQ_E2,	SQ_F2,	SQ_G2,	SQ_H2,
	SQ_A3,	SQ_B3,	SQ_C3,	SQ_D3,	SQ_E3,	SQ_F3,	SQ_G3,	SQ_H3,
	SQ_A4,	SQ_B4,	SQ_C4,	SQ_D4,	SQ_E4,	SQ_F4,	SQ_G4,	SQ_H4,
	SQ_A5,	SQ_B5,	SQ_C5,	SQ_D5,	SQ_E5,	SQ_F5,	SQ_G5,	SQ_H5,
	SQ_A6,	SQ_B6,	SQ_C6,	SQ_D6,	SQ_E6,	SQ_F6,	SQ_G6,	SQ_H6,
	SQ_A7,	SQ_B7,	SQ_C7,	SQ_D7,	SQ_E7,	SQ_F7,	SQ_G7,	SQ_H7,
	SQ_A8,	SQ_B8,	SQ_C8,	SQ_D8,	SQ_E8,	SQ_F8,	SQ_G8,	SQ_H8,
	MOVE_NULL = 65,
	TO_KNIGHT = 0 << 12,
	TO_BISHOP = 1 << 12,
	TO_ROOK = 2 << 12,
	TO_QUEEN = 3 << 12,
	PROMOTION_MASK = 3 << 12,
	PROMOTION = 1 << 14,
	EN_PASSANT = 2 << 14,
	CASTLING = 3 << 14,
	SPE_MASK = 3 << 14
};

enum	e_special_moves
{
	WHITE_OO = 1,
	WHITE_OOO = 2,
	BLACK_OO = 4,
	BLACK_OOO = 8,

	KING_SIDE = 5,
	QUEEN_SIDE = 10,
	WHITE_CASTLING = 3,
	BLACK_CASTLING = 12,
	ANY_CASTLING = 15,

	EN_PASSANT_OK = 128
};

enum	e_direction
{
	NORTH = 8,
	SOUTH = -8,
	EAST = 1,
	WEST = -1,
	NORTH_EAST = 9,
	SOUTH_EAST = -7,
	NORTH_WEST = 7,
	SOUTH_WEST = -9
};

enum	e_file
{
	FILE_A,	FILE_B,	FILE_C,	FILE_D,	FILE_E,	FILE_F,	FILE_G,	FILE_H
};

enum	e_rank
{
	RANK_1,	RANK_2,	RANK_3,	RANK_4,	RANK_5,	RANK_6,	RANK_7,	RANK_8
};

enum	e_piece
{
	PIECE_NONE,
	WHITE_PAWN = 1,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN = 9,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING
};

enum	e_player
{
	PLAYER_WHITE,
	PLAYER_BLACK
};

typedef int16_t	move_t;
typedef int16_t	value_t;

enum
{
	VALUE_MATE_FOUND = 30000,
	VALUE_MATE = 32000,
	VALUE_INFINITE = 32001,
	NO_VALUE = INT16_MIN
};

typedef struct	movelist_s
{
	size_t	size;
	move_t	*moves;
	value_t	*values;
}				movelist_t;

extern pthread_mutex_t	mtx_engine;
extern pthread_cond_t	cv_engine;

extern enum e_egn_mode	g_engine_mode;
extern enum e_egn_send	g_engine_send;

extern clock_t		g_wtime;
extern clock_t		g_btime;
extern clock_t		g_winc;
extern clock_t		g_binc;
extern int			g_movestogo;
extern int			g_depth;
extern int			g_curdepth;
extern size_t		g_nodes;
extern _Atomic size_t	g_curnodes;
extern int			g_mate;
extern clock_t		g_movetime;
extern clock_t		g_start;
extern int			g_infinite;

extern board_t		g_init_board;
extern movelist_t	*g_inter_moves;
extern board_t		g_real_board;
extern movelist_t	*g_searchmoves;
extern int16_t		*g_valuemoves;

inline move_t	get_move(int16_t from, int16_t to)
{
	return ((from << 6) | to);
}

inline int8_t	vt_square(int8_t sq)
{
	return ((sq & 7) | ((7 - (sq >> 3)) << 3));
}

inline int8_t	ht_square(int8_t sq)
{
	return ((sq & 56) | (7 - sq));
}

inline int16_t	move_from(move_t move)
{
	return ((move >> 6) & 63);
}

inline int16_t	move_to(move_t move)
{
	return (move & 63);
}

inline clock_t	chess_clock(void)
{
	struct timespec	tp;

	clock_gettime(CLOCK_REALTIME, &tp);
	return ((clock_t)tp.tv_sec * 1000 +
			(clock_t)tp.tv_nsec / 1000000);
}

move_t		str_to_move(const char *str);
char		*move_to_str(move_t move);
void		do_move(board_t *board, move_t move);
movelist_t	*movelist_init(void);
void		push_move(movelist_t *mlist, move_t move);
void		pop_move(movelist_t *mlist, size_t index);
void		movelist_quit(movelist_t *mlist);

void		launch_analyse(void);
void		*analysis_thread(void *index);

int16_t		eval_board(board_t *board, int depth);

movelist_t	*get_simple_moves(board_t *board);

void		get_pawn_moves(movelist_t *mlist, int8_t sq, board_t *board);
void		get_knight_moves(movelist_t *mlist, int8_t sq, board_t *board);
void		get_bishop_moves(movelist_t *mlist, int8_t sq, board_t *board);
void		get_rook_moves(movelist_t *mlist, int8_t sq, board_t *board);
void		get_king_moves(movelist_t *mlist, int8_t sq, board_t *board);

int			is_checked(const board_t *board);

#endif
