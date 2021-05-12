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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imath.h"
#include "tuner.h"

void start_tuning_session(const char *filename)
{
#ifdef TUNE
    tp_vector_t delta = {}, base = {};
    double K, mse, lr = LEARNING_RATE;
    tune_data_t data = {};

    init_base_values(base);
    init_tuner_entries(&data, filename);
    K = compute_optimal_k(&data);

    for (int iter = 0; iter < ITERS; ++iter)
    {
        for (int batchIdx = 0; (size_t)batchIdx < data.size / BATCH_SIZE; ++batchIdx)
        {
            tp_vector_t gradient = {};
            compute_gradient(&data, gradient, delta, K, batchIdx);

            const double scale = (K * 2.0 / BATCH_SIZE) * lr;

            for (int i = 0; i < IDX_COUNT; ++i)
            {
                delta[i][MIDGAME] += gradient[i][MIDGAME] * scale;
                delta[i][ENDGAME] += gradient[i][ENDGAME] * scale;
            }
        }

        mse = adjusted_eval_mse(&data, delta, K);
        printf("Iteration [%d], MSE [%g], LR [%g]\n", iter, mse, lr);

        if (iter % LR_DROP_ITERS == LR_DROP_ITERS - 1)
            lr /= LR_DROP_VALUE;
        if (iter % 50 == 49)
            print_parameters(base, delta);
    }

    for (size_t i = 0; i < data.size; ++i)
        free(data.entries[i].tuples);
    free(data.entries);
#endif
}

#ifdef TUNE
void init_base_values(tp_vector_t base)
{
#define INIT_BASE_SP(idx, val) do { \
    extern const scorepair_t val; \
    base[idx][MIDGAME] = midgame_score(val); \
    base[idx][ENDGAME] = endgame_score(val); \
} while (0)

#define INIT_BASE_SPA(idx, val, size) do { \
    extern const scorepair_t val[size]; \
    for (int i = 0; i < size; ++i) \
    { \
        base[idx][MIDGAME] = midgame_score(val[i]); \
        base[idx][ENDGAME] = endgame_score(val[i]); \
    } \
} while (0)

    extern const scorepair_t PawnBonus[RANK_NB][FILE_NB];
    extern const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2];

    base[IDX_PIECE + 0][MIDGAME] = PAWN_MG_SCORE;
    base[IDX_PIECE + 0][ENDGAME] = PAWN_EG_SCORE;
    base[IDX_PIECE + 1][MIDGAME] = KNIGHT_MG_SCORE;
    base[IDX_PIECE + 1][ENDGAME] = KNIGHT_EG_SCORE;
    base[IDX_PIECE + 2][MIDGAME] = BISHOP_MG_SCORE;
    base[IDX_PIECE + 2][ENDGAME] = BISHOP_EG_SCORE;
    base[IDX_PIECE + 3][MIDGAME] = ROOK_MG_SCORE;
    base[IDX_PIECE + 3][ENDGAME] = ROOK_EG_SCORE;
    base[IDX_PIECE + 4][MIDGAME] = QUEEN_MG_SCORE;
    base[IDX_PIECE + 4][ENDGAME] = QUEEN_EG_SCORE;

    for (square_t sq = SQ_A2; sq <= SQ_H7; ++sq)
    {
        scorepair_t sp = PawnBonus[sq_rank(sq)][sq_file(sq)];

        base[IDX_PSQT + sq - SQ_A2][MIDGAME] = midgame_score(sp);
        base[IDX_PSQT + sq - SQ_A2][ENDGAME] = endgame_score(sp);
    }

    for (piecetype_t pt = KNIGHT; pt <= KING; ++pt)
        for (square_t sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            scorepair_t sp = PieceBonus[pt][sq_rank(sq)][min(sq_file(sq), sq_file(sq) ^ 7)];

            base[IDX_PSQT + 48 + 64 * (pt - KNIGHT) + sq][MIDGAME] = midgame_score(sp);
            base[IDX_PSQT + 48 + 64 * (pt - KNIGHT) + sq][ENDGAME] = endgame_score(sp);
        }

    INIT_BASE_SP(IDX_CASTLING, CastlingBonus);
    INIT_BASE_SP(IDX_INITIATIVE, Initiative);

    INIT_BASE_SP(IDX_KS_KNIGHT, KnightWeight);
    INIT_BASE_SP(IDX_KS_BISHOP, BishopWeight);
    INIT_BASE_SP(IDX_KS_ROOK, RookWeight);
    INIT_BASE_SP(IDX_KS_QUEEN, QueenWeight);

    INIT_BASE_SP(IDX_KNIGHT_SHIELDED, KnightShielded);
    INIT_BASE_SP(IDX_KNIGHT_OUTPOST, KnightOutpost);
    INIT_BASE_SP(IDX_KNIGHT_CENTER_OUTPOST, KnightCenterOutpost);
    INIT_BASE_SP(IDX_KNIGHT_SOLID_OUTPOST, KnightSolidOutpost);

    INIT_BASE_SP(IDX_BISHOP_PAIR, BishopPairBonus);
    INIT_BASE_SP(IDX_BISHOP_SHIELDED, BishopShielded);

    INIT_BASE_SP(IDX_ROOK_SEMIOPEN, RookOnSemiOpenFile);
    INIT_BASE_SP(IDX_ROOK_OPEN, RookOnOpenFile);
    INIT_BASE_SP(IDX_ROOK_XRAY_QUEEN, RookXrayQueen);

    INIT_BASE_SPA(IDX_MOBILITY_KNIGHT, MobilityN, 9);
    INIT_BASE_SPA(IDX_MOBILITY_BISHOP, MobilityB, 14);
    INIT_BASE_SPA(IDX_MOBILITY_ROOK, MobilityR, 15);
    INIT_BASE_SPA(IDX_MOBILITY_QUEEN, MobilityQ, 28);

    INIT_BASE_SP(IDX_BACKWARD, BackwardPenalty);
    INIT_BASE_SP(IDX_STRAGGLER, StragglerPenalty);
    INIT_BASE_SP(IDX_DOUBLED, DoubledPenalty);
    INIT_BASE_SP(IDX_ISOLATED, IsolatedPenalty);

    extern const scorepair_t PassedBonus[RANK_NB];

    for (rank_t r = RANK_2; r <= RANK_7; ++r)
    {
        base[IDX_PASSER + r - RANK_2][MIDGAME] = midgame_score(PassedBonus[r]);
        base[IDX_PASSER + r - RANK_2][ENDGAME] = endgame_score(PassedBonus[r]);
    }
}

void init_tuner_entries(tune_data_t *data, const char *filename)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
    {
        perror("Unable to open dataset");
        exit(EXIT_FAILURE);
    }

    board_t board = {};
    boardstack_t stack = {};
    char linebuf[1024];

    while (fgets(linebuf, sizeof(linebuf), f) != NULL)
    {
        if (data->maxSize == data->size)
        {
            data->maxSize += !data->maxSize ? 16 : data->maxSize / 2;
            data->entries = realloc(data->entries, sizeof(tune_entry_t) * data->maxSize);

            if (data->entries == NULL)
            {
                perror("Unable to allocate dataset entries");
                exit(EXIT_FAILURE);
            }
        }

        tune_entry_t *cur = &data->entries[data->size];

        char *ptr = strrchr(linebuf, ' ');

        *ptr = '\0';
        if (sscanf(ptr + 1, "%lf", &cur->gameResult) == 0)
        {
            fputs("Unable to read game result\n", stdout);
            exit(EXIT_FAILURE);
        }

        set_board(&board, linebuf, false, &stack);
        init_tuner_entry(cur, &board);
    }
}

void init_tuner_entry(tune_entry_t *entry, const board_t *board)
{
    entry->staticEval = evaluate(board);
    if (board->sideToMove == WHITE)
        entry->staticEval = -entry->staticEval;

    entry->phase = Trace.phase;
    entry->phaseFactors[MIDGAME] = entry->phase / 24.0;
    entry->phaseFactors[ENDGAME] = 1 - entry->phaseFactors[MIDGAME];

    init_tuner_tuples(entry);

    entry->eval = Trace.eval;
    entry->safetyAttackers[WHITE] = Trace.safetyAttackers[WHITE];
    entry->safetyAttackers[BLACK] = Trace.safetyAttackers[BLACK];
    entry->scaleFactor = Trace.scaleFactor / 128.0;
    entry->sideToMove = board->sideToMove;
}

void init_tuner_tuples(tune_entry_t *entry)
{
    (void)entry;
}

#endif
