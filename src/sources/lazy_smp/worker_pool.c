#include <stdio.h>
#include <stdlib.h>
#include "lazy_smp.h"

worker_pool_t	WPool;

void	wpool_init(int threads)
{
	wpool_quit();

	WPool.size = threads;
	WPool.list = malloc(sizeof(worker_t) * threads);
	if (WPool.list == NULL)
	{
		perror("Unable to allocate worker pool");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < threads; ++i)
	{
		worker_t	*worker = WPool.list + i;

		worker->idx = i;
		worker->stack = NULL;
	}

	extern board_t	g_board;

	g_board.worker = WPool.list;
}

void	wpool_quit(void)
{
	free(WPool.list);
	WPool.list = NULL;
	WPool.size = 0;
}
