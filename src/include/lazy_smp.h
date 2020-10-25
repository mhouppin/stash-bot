#ifndef LAZY_SMP_H
# define LAZY_SMP_H

# include <pthread.h>
# include "board.h"
# include "history.h"
# include "pawns.h"

// Struct for worker thread data.

typedef struct
{
	int					idx;

	board_t				board;
	boardstack_t		*stack;
	history_t			good_history;
	history_t			bad_history;
	pawns_table_t		pawns_cache;

	int					seldepth;
	int					verif_plies;
	_Atomic uint64_t	nodes;

	pthread_t			thread;
}
worker_t;

typedef struct
{
	int			size;
	worker_t	*list;
}
worker_pool_t;

extern worker_pool_t	WPool;

INLINED worker_t	*get_worker(const board_t *board)
{
	return (board->worker);
}

INLINED uint64_t	get_node_count(void)
{
	uint64_t	result = 0;

	for (int i = 0; i < WPool.size; ++i)
		result += WPool.list[i].nodes;

	return (result);
}

void	wpool_init(int threads);
void	wpool_quit(void);

#endif
