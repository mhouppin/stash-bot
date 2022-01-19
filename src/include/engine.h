/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ENGINE_H
# define ENGINE_H

# include <time.h>
# include "board.h"
# include "history.h"
# include "movepick.h"

# ifdef TUNE

typedef enum tune_idx_e
{
    IDX_PIECE,
    IDX_PSQT = IDX_PIECE + 5,
    IDX_CASTLING = IDX_PSQT + 48 + 32 * 5,
    IDX_INITIATIVE,
    IDX_KNIGHT_SHIELDED,
    IDX_KNIGHT_OUTPOST,
    IDX_KNIGHT_CENTER_OUTPOST,
    IDX_KNIGHT_SOLID_OUTPOST,
    IDX_BISHOP_PAIR,
    IDX_BISHOP_SHIELDED,
    IDX_ROOK_SEMIOPEN,
    IDX_ROOK_OPEN,
    IDX_ROOK_XRAY_QUEEN,
    IDX_MOBILITY_KNIGHT,
    IDX_MOBILITY_BISHOP = IDX_MOBILITY_KNIGHT + 9,
    IDX_MOBILITY_ROOK = IDX_MOBILITY_BISHOP + 14,
    IDX_MOBILITY_QUEEN = IDX_MOBILITY_ROOK + 15,
    IDX_BACKWARD = IDX_MOBILITY_QUEEN + 28,
    IDX_STRAGGLER,
    IDX_DOUBLED,
    IDX_ISOLATED,
    IDX_PASSER,
    IDX_PHALANX = IDX_PASSER + 6,
    IDX_DEFENDER = IDX_PHALANX + 6,
    IDX_PP_OUR_KING_PROX = IDX_DEFENDER + 5,
    IDX_PP_THEIR_KING_PROX = IDX_PP_OUR_KING_PROX + 7,
    IDX_KS_KNIGHT = IDX_PP_THEIR_KING_PROX + 7,
    IDX_KS_BISHOP,
    IDX_KS_ROOK,
    IDX_KS_QUEEN,
    IDX_KS_ATTACK,
    IDX_KS_WEAK_Z,
    IDX_KS_CHECK_N,
    IDX_KS_CHECK_B,
    IDX_KS_CHECK_R,
    IDX_KS_CHECK_Q,
    IDX_KS_QUEENLESS,
    IDX_KS_OFFSET,
    IDX_COUNT
}
tune_idx_t;

typedef struct evaltrace_s
{
    int phase;
    scorepair_t eval;
    scorepair_t safety[COLOR_NB];
    int scaleFactor;
    int8_t coeffs[IDX_COUNT][COLOR_NB];
} evaltrace_t;

extern evaltrace_t Trace;

# define TRACE_INIT memset(&Trace, 0, sizeof(Trace))
# define TRACE_ADD(idx, color, n) Trace.coeffs[idx][color] += n
# define TRACE_PHASE(p) Trace.phase = p
# define TRACE_SAFETY(c, v) Trace.safety[c] = v
# define TRACE_EVAL(e) Trace.eval = e
# define TRACE_FACTOR(f) Trace.scaleFactor = f

# else

# define TRACE_INIT
# define TRACE_ADD(x, c, n)
# define TRACE_PHASE(p)
# define TRACE_SAFETY(c, v)
# define TRACE_EVAL(e)
# define TRACE_FACTOR(f)

# endif

enum { MAX_PLIES = 240 };

extern int Reductions[64][64];
extern int Pruning[2][7];

void sort_root_moves(root_move_t *begin, root_move_t *end);
root_move_t *find_root_move(root_move_t *begin, root_move_t *end, move_t move);

void *engine_go(void *ptr);
score_t qsearch(board_t *board, score_t alpha, score_t beta, searchstack_t *ss, bool pvNode);
score_t search(board_t *board, int depth, score_t alpha, score_t beta,
    searchstack_t *ss, bool pvNode);

void update_quiet_history(const board_t *board, int depth,
    move_t bestmove, const move_t quiets[64], int qcount, searchstack_t *ss);
void update_capture_history(const board_t *board, int depth,
    move_t bestmove, const move_t captures[64], int ccount, searchstack_t *ss);

score_t evaluate(const board_t *board);
score_t scale_endgame(const board_t *board, score_t eg);

void init_reduction_table(void);

void *engine_thread(void *nothing);

#endif
