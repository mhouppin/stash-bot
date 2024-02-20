/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2024 Morgan Houppin
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

#include "endgame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    KPK_SIZE = 2 * 24 * 64 * 64,

    KPK_INVALID = 0,
    KPK_UNKNOWN = 1,
    KPK_DRAW = 2,
    KPK_WIN = 4
};

typedef struct kpk_position_s
{
    color_t sideToMove;
    square_t kingSquare[COLOR_NB];
    square_t pawnSquare;
    uint8_t result;
} kpk_position_t;

uint8_t KPK_Bitbase[KPK_SIZE / 8];

INLINED unsigned int kpk_index(color_t stm, square_t bksq, square_t wksq, square_t psq)
{
    return (unsigned int)wksq | ((unsigned int)bksq << 6) | ((unsigned int)stm << 12)
           | ((unsigned int)sq_file(psq) << 13) | ((unsigned int)(RANK_7 - sq_rank(psq)) << 15);
}

bool kpk_is_winning(color_t stm, square_t bksq, square_t wksq, square_t psq)
{
    unsigned int index = kpk_index(stm, bksq, wksq, psq);

    return KPK_Bitbase[index >> 3] & (1 << (index & 7));
}

void kpk_set(kpk_position_t *pos, unsigned int index)
{
    const square_t wksq = (square_t)(index & 0x3F);
    const square_t bksq = (square_t)((index >> 6) & 0x3F);
    const color_t stm = (color_t)((index >> 12) & 1);
    const square_t psq =
        create_sq((file_t)((index >> 13) & 0x3), (rank_t)(RANK_7 - ((index >> 15) & 0x7)));

    pos->sideToMove = stm;
    pos->kingSquare[WHITE] = wksq;
    pos->kingSquare[BLACK] = bksq;
    pos->pawnSquare = psq;

    // Overlapping/adjacent Kings ?
    if (SquareDistance[wksq][bksq] <= 1) pos->result = KPK_INVALID;

    // Overlapping King with Pawn ?
    else if (wksq == psq || bksq == psq)
        pos->result = KPK_INVALID;

    // Losing king in check while the winning side has the move ?
    else if (stm == WHITE && (PawnMoves[WHITE][psq] & square_bb(bksq)))
        pos->result = KPK_INVALID;

    // Can we promote without getting captured ?
    else if (stm == WHITE && sq_rank(psq) == RANK_7 && wksq != psq + NORTH
             && (SquareDistance[bksq][psq + NORTH] > 1 || SquareDistance[wksq][psq + NORTH] == 1))
        pos->result = KPK_WIN;

    // Is it stalemate ?
    else if (stm == BLACK && !(king_moves(bksq) & ~(king_moves(wksq) | PawnMoves[WHITE][psq])))
        pos->result = KPK_DRAW;

    // Can the losing side capture the Pawn ?
    else if (stm == BLACK && (king_moves(bksq) & ~king_moves(wksq) & square_bb(psq)))
        pos->result = KPK_DRAW;

    else
        pos->result = KPK_UNKNOWN;
}

void kpk_classify(kpk_position_t *pos, kpk_position_t *kpkTable)
{
    const uint8_t goodResult = (pos->sideToMove == WHITE) ? KPK_WIN : KPK_DRAW;
    const uint8_t badResult = (pos->sideToMove == WHITE) ? KPK_DRAW : KPK_WIN;

    const square_t wksq = pos->kingSquare[WHITE];
    const square_t bksq = pos->kingSquare[BLACK];
    const color_t stm = pos->sideToMove;
    const square_t psq = pos->pawnSquare;

    uint8_t result = KPK_INVALID;
    bitboard_t b = king_moves(pos->kingSquare[stm]);

    // We will pack all moves' results in the result variable with bitwise 'or's.
    // We exploit the fact that invalid entries with overlapping pieces are stored
    // as KPK_INVALID (aka 0) to avoid checking for move legality (expect for double
    // Pawn pushes, where we need to check if the square above the pawn is empty).

    // Get all entries' results for King moves.
    while (b)
    {
        if (stm == WHITE)
            result |= kpkTable[kpk_index(BLACK, bksq, bb_pop_first_sq(&b), psq)].result;
        else
            result |= kpkTable[kpk_index(WHITE, bb_pop_first_sq(&b), wksq, psq)].result;
    }

    // If the winning side has the move, also get all entries' results for Pawn moves.
    if (stm == WHITE)
    {
        // Single push
        if (sq_rank(psq) < RANK_7)
            result |= kpkTable[kpk_index(BLACK, bksq, wksq, psq + NORTH)].result;

        // Double push
        if (sq_rank(psq) == RANK_2 && psq + NORTH != wksq && psq + NORTH != bksq)
            result |= kpkTable[kpk_index(BLACK, bksq, wksq, psq + NORTH + NORTH)].result;
    }

    pos->result = ((result & goodResult)    ? goodResult
                   : (result & KPK_UNKNOWN) ? KPK_UNKNOWN
                                            : badResult);
}

void init_kpk_bitbase(void)
{
    kpk_position_t *kpkTable = malloc(sizeof(kpk_position_t) * KPK_SIZE);

    if (kpkTable == NULL)
    {
        perror("Unable to initialize KPK bitbase");
        exit(EXIT_FAILURE);
    }

    unsigned int index;
    bool repeat;

    // Fill the bitbase with zeroes, and then perform an early
    // recognition of trivial wins/draws and illegal positions.
    memset(KPK_Bitbase, 0, sizeof(KPK_Bitbase));
    for (index = 0; index < KPK_SIZE; ++index) kpk_set(kpkTable + index, index);

    // Backtrack all known wins/draws to the undecided positions, by trying to
    // determine the result of a position from the child states' results.
    do {
        repeat = false;
        for (index = 0; index < KPK_SIZE; ++index)
            if (kpkTable[index].result == KPK_UNKNOWN)
            {
                kpk_classify(kpkTable + index, kpkTable);
                repeat |= kpkTable[index].result != KPK_UNKNOWN;
            }
    } while (repeat);

    // Index the wins in the bitbase as set bits.
    for (index = 0; index < KPK_SIZE; ++index)
        if (kpkTable[index].result == KPK_WIN) KPK_Bitbase[index / 8] |= 1 << (index % 8);

    free(kpkTable);
}

score_t eval_kpk(const Board *board, color_t winningSide)
{
    square_t winningKing = get_king_square(board, winningSide);
    square_t winningPawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t losingKing = get_king_square(board, not_color(winningSide));
    color_t us = winningSide == board->sideToMove ? WHITE : BLACK;

    winningKing = normalize_square(board, winningSide, winningKing);
    winningPawn = normalize_square(board, winningSide, winningPawn);
    losingKing = normalize_square(board, winningSide, losingKing);

    // Probe the bitbase to evaluate the KPK position.
    score_t score = kpk_is_winning(us, losingKing, winningKing, winningPawn)
                        ? VICTORY + PAWN_EG_SCORE + sq_rank(winningPawn) * 3
                        : 0;

    return winningSide == board->sideToMove ? score : -score;
}
