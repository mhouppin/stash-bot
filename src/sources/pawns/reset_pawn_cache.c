#include <string.h>
#include "pawns.h"

void	reset_pawn_cache(void)
{
	memset(g_pawns, 0, sizeof(g_pawns));
}
