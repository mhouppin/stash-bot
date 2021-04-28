/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "endgame.h"

enum
{
    KPK_SIZE = 2 * 24 * 64 * 64,

    KPK_INVALID = 0,
    KPK_UNKNOWN = 1,
    KPK_DRAW = 2,
    KPK_WIN = 4
};

typedef struct
{
    color_t     side_to_move;
    square_t    king_square[COLOR_NB];
    square_t    pawn_square;
    uint8_t     result;
}
kpk_position_t;

uint8_t KPK_Bitbase[KPK_SIZE / 8];

INLINED unsigned int kpk_index(color_t stm, square_t bksq, square_t wksq, square_t psq)
{
    return (
            (unsigned int)wksq
            | ((unsigned int)bksq << 6)
            | ((unsigned int)stm << 12)
            | ((unsigned int)sq_file(psq) << 13)
            | ((unsigned int)(RANK_7 - sq_rank(psq)) << 15)
            );
}

INLINED bool kpk_is_winning(color_t stm, square_t bksq, square_t wksq, square_t psq)
{
    unsigned int    index = kpk_index(stm, bksq, wksq, psq);

    return (KPK_Bitbase[index >> 3] & (1 << (index & 7)));
}

void    kpk_set(kpk_position_t *pos, unsigned int index)
{
    const square_t  wksq = (square_t)(index & 0x3F);
    const square_t  bksq = (square_t)((index >> 6) & 0x3F);
    const color_t   stm = (color_t)((index >> 12) & 1);
    const square_t  psq = create_sq((file_t)((index >> 13) & 0x3),
            (rank_t)(RANK_7 - ((index >> 15) & 0x7)));

    pos->side_to_move = stm;
    pos->king_square[WHITE] = wksq;
    pos->king_square[BLACK] = bksq;
    pos->pawn_square = psq;

    // Overlapping/adjacent Kings ?
    if (SquareDistance[wksq][bksq] <= 1)
        pos->result = KPK_INVALID;

    // Overlapping King with Pawn ?
    else if (wksq == psq || bksq == psq)
        pos->result = KPK_INVALID;

    // Losing king in check while the winning side has the move ?
    else if (stm == WHITE && (PawnMoves[WHITE][psq] & square_bb(bksq)))
        pos->result = KPK_INVALID;

    // Can we promote without getting captured ?
    else if (stm == WHITE
            && sq_rank(psq) == RANK_7
            && wksq != psq + NORTH
            && (SquareDistance[bksq][psq + NORTH] > 1
                || SquareDistance[wksq][psq + NORTH] == 1))
        pos->result = KPK_WIN;

    // Is it stalemate ?
    else if (stm == BLACK
            && !(king_moves(bksq) & ~(king_moves(wksq) | PawnMoves[WHITE][psq])))
        pos->result = KPK_DRAW;

    // Can the losing side capture the Pawn ?
    else if (stm == BLACK && (king_moves(bksq) & ~king_moves(wksq) & square_bb(psq)))
        pos->result = KPK_DRAW;

    else
        pos->result = KPK_UNKNOWN;
}

void    kpk_classify(kpk_position_t *pos, kpk_position_t *kpk_table)
{
    const uint8_t   good_result = (pos->side_to_move == WHITE) ? KPK_WIN : KPK_DRAW;
    const uint8_t   bad_result = (pos->side_to_move == WHITE) ? KPK_DRAW : KPK_WIN;

    const square_t  wksq = pos->king_square[WHITE];
    const square_t  bksq = pos->king_square[BLACK];
    const color_t   stm = pos->side_to_move;
    const square_t  psq = pos->pawn_square;

    uint8_t         r = KPK_INVALID;
    bitboard_t      b = king_moves(pos->king_square[stm]);

    // We will pack all moves' results in the result variable with bitwise 'or's.
    // We exploit the fact that invalid entries with overlapping pieces are stored
    // as KPK_INVALID (aka 0) to avoid checking for move legality (expect for double
    // pawn pushes, where we need to check if the square above the pawn is empty).

    // Get all entries' results for king moves
    while (b)
    {
        if (stm == WHITE)
            r |= kpk_table[kpk_index(BLACK, bksq, bb_pop_first_sq(&b), psq)].result;
        else
            r |= kpk_table[kpk_index(WHITE, bb_pop_first_sq(&b), wksq, psq)].result;
    }

    // If the winning side has the move, also get all entries' results for pawn moves
    if (stm == WHITE)
    {
        // Single push
        if (sq_rank(psq) < RANK_7)
            r |= kpk_table[kpk_index(BLACK, bksq, wksq, psq + NORTH)].result;

        // Double push
        if (sq_rank(psq) == RANK_2 && psq + NORTH != wksq && psq + NORTH != bksq)
            r |= kpk_table[kpk_index(BLACK, bksq, wksq, psq + NORTH + NORTH)].result;
    }

    pos->result = (r & good_result ? good_result : r & KPK_UNKNOWN ? KPK_UNKNOWN : bad_result);
}

void    init_kpk_bitbase(void)
{
    kpk_position_t  *kpk_table;

    kpk_table = malloc(sizeof(kpk_position_t) * KPK_SIZE);

    if (kpk_table == NULL)
    {
        perror("Unable to initialize KPK bitbase");
        exit(EXIT_FAILURE);
    }

    unsigned int    index;
    bool            repeat;

    memset(KPK_Bitbase, 0, sizeof(KPK_Bitbase));

    for (index = 0; index < KPK_SIZE; ++index)
        kpk_set(kpk_table + index, index);

    do
    {
        repeat = false;
        for (index = 0; index < KPK_SIZE; ++index)
            if (kpk_table[index].result == KPK_UNKNOWN)
            {
                kpk_classify(kpk_table + index, kpk_table);
                repeat |= kpk_table[index].result != KPK_UNKNOWN;
            }
    }
    while (repeat);

    for (index = 0; index < KPK_SIZE; ++index)
        if (kpk_table[index].result == KPK_WIN)
            KPK_Bitbase[index / 8] |= 1 << (index % 8);

    free(kpk_table);
}

score_t eval_kpk(const board_t *board, color_t winning_side)
{
    square_t    winning_king = get_king_square(board, winning_side);
    square_t    winning_pawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t    losing_king = get_king_square(board, not_color(winning_side));
    color_t     us = winning_side == board->side_to_move ? WHITE : BLACK;

    winning_king = normalize_square(board, winning_side, winning_king);
    winning_pawn = normalize_square(board, winning_side, winning_pawn);
    losing_king = normalize_square(board, winning_side, losing_king);

    score_t     score = kpk_is_winning(us, losing_king, winning_king, winning_pawn)
        ? VICTORY + PAWN_EG_SCORE + sq_rank(winning_pawn) * 3 : 0;

    return (winning_side == board->side_to_move ? score : -score);
}