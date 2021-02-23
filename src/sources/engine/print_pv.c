#include <stdio.h>
#include "engine.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "tt.h"

const char  *BoundStr[] = {
    "",
    " upperbound",
    " lowerbound",
    ""
};

void    print_pv(const board_t *board, root_move_t *root_move, int multi_pv,
        int depth, clock_t time, int bound)
{
    uint64_t    nodes = get_node_count();
    uint64_t    nps = nodes / (time + !time) * 1000;
    bool        searched_move = (root_move->score != -INF_SCORE);
    score_t     root_score = (searched_move) ? root_move->score : root_move->previous_score;

    printf("info depth %d seldepth %d multipv %d score %s%s", max(depth + searched_move, 1),
        root_move->seldepth, multi_pv, score_to_str(root_score), BoundStr[bound]);
    printf(" nodes %" FMT_INFO " nps %" FMT_INFO " hashfull %d time %" FMT_INFO " pv",
        (info_t)nodes, (info_t)nps, tt_hashfull(), (info_t)time);

    for (size_t k = 0; root_move->pv[k]; ++k)
        printf(" %s", move_to_str(root_move->pv[k], board->chess960));
    putchar('\n');
}
