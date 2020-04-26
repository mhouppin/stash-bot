#ifndef PAWNS_H
# define PAWNS_H

# include "board.h"

typedef struct	pawns_cache_s
{
	hashkey_t	key;
	scorepair_t	value;
}
pawns_cache_t;

enum
{
	PawnCacheSize = 4096
};

extern pawns_cache_t	g_pawns[PawnCacheSize];

void			reset_pawn_cache(void);
scorepair_t		evaluate_pawns(const board_t *board);

#endif
