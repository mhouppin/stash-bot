#include "engine.h"

void	update_quiet_history(history_t hist, const board_t *board, int depth,
		move_t bestmove, const move_t quiets[64], int qcount, searchstack_t *ss)
{
	int		bonus = (depth <= 12) ? 16 * depth * depth : 20;

	add_history(hist, piece_on(board, move_from_square(bestmove)),
		bestmove, bonus);

	if (ss->killers[0] == NO_MOVE)
		ss->killers[0] = bestmove;
	else if (ss->killers[0] != bestmove)
		ss->killers[1] = bestmove;

	for (int i = 0; i < qcount; ++i)
		add_history(hist, piece_on(board, move_from_square(quiets[i])),
		quiets[i], -bonus);
}
