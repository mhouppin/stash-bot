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

#include "tuner.h"
#include "types.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_tuning_session(const char *filename)
{
#ifdef TUNE
    tp_vector_t delta = {}, base = {}, momentumGrad = {}, velocityGrad = {};
    double K, lr = LEARNING_RATE;
    tune_data_t data = {};

    init_base_values(base);
    init_tuner_entries(&data, filename);
    K = compute_optimal_k(&data);

    size_t batches = data.size / BATCH_SIZE;

    for (int iter = 0; iter < ITERS; ++iter)
    {
        for (int batchIdx = 0; (size_t)batchIdx < batches; ++batchIdx)
        {
            tp_vector_t gradient = {};
            compute_gradient(&data, gradient, delta, K, batchIdx);

            const double scale = (K * 2.0 / BATCH_SIZE);

            for (int i = 0; i < IDX_COUNT; ++i)
            {
                double mgGrad = gradient[i][MIDGAME] * scale;
                double egGrad = gradient[i][ENDGAME] * scale;

                momentumGrad[i][MIDGAME] = momentumGrad[i][MIDGAME] * 0.9 + mgGrad * 0.1;
                momentumGrad[i][ENDGAME] = momentumGrad[i][ENDGAME] * 0.9 + egGrad * 0.1;

                velocityGrad[i][MIDGAME] =
                    velocityGrad[i][MIDGAME] * 0.999 + pow(mgGrad, 2.0) * 0.001;
                velocityGrad[i][ENDGAME] =
                    velocityGrad[i][ENDGAME] * 0.999 + pow(egGrad, 2.0) * 0.001;

                delta[i][MIDGAME] +=
                    momentumGrad[i][MIDGAME] * lr / sqrt(1e-8 + velocityGrad[i][MIDGAME]);
                delta[i][ENDGAME] +=
                    momentumGrad[i][ENDGAME] * lr / sqrt(1e-8 + velocityGrad[i][ENDGAME]);
            }
        }

        double loss = adjusted_eval_mse(&data, delta, K);
        printf("Iteration [%d], Loss [%.7f]\n", iter, loss);

        if (iter % LR_DROP_ITERS == LR_DROP_ITERS - 1) lr /= LR_DROP_VALUE;

        if (iter % 50 == 49 || iter == ITERS - 1) print_parameters(base, delta);

        fflush(stdout);
    }

    for (size_t i = 0; i < data.size; ++i) free(data.entries[i].tuples);
    free(data.entries);
#else
    (void)filename;
#endif
}

#ifdef TUNE
void init_base_values(tp_vector_t base)
{
#define INIT_BASE_SP(idx, val)                   \
    do {                                         \
        extern const scorepair_t val;            \
        base[idx][MIDGAME] = midgame_score(val); \
        base[idx][ENDGAME] = endgame_score(val); \
    } while (0)

#define INIT_BASE_SPA(idx, val, size)                       \
    do {                                                    \
        extern const scorepair_t val[size];                 \
        for (int i = 0; i < size; ++i)                      \
        {                                                   \
            base[idx + i][MIDGAME] = midgame_score(val[i]); \
            base[idx + i][ENDGAME] = endgame_score(val[i]); \
        }                                                   \
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
        for (file_t file = FILE_A; file <= FILE_D; ++file)
            for (rank_t rank = RANK_1; rank <= RANK_8; ++rank)
            {
                scorepair_t sp = PieceBonus[pt][rank][file];

                base[IDX_PSQT + 48 + 32 * (pt - KNIGHT) + rank * 4 + file][MIDGAME] =
                    midgame_score(sp);
                base[IDX_PSQT + 48 + 32 * (pt - KNIGHT) + rank * 4 + file][ENDGAME] =
                    endgame_score(sp);
            }

    INIT_BASE_SP(IDX_CASTLING, CastlingBonus);
    INIT_BASE_SP(IDX_INITIATIVE, Initiative);

    INIT_BASE_SP(IDX_KS_KNIGHT, KnightWeight);
    INIT_BASE_SP(IDX_KS_BISHOP, BishopWeight);
    INIT_BASE_SP(IDX_KS_ROOK, RookWeight);
    INIT_BASE_SP(IDX_KS_QUEEN, QueenWeight);
    INIT_BASE_SP(IDX_KS_ATTACK, AttackWeight);
    INIT_BASE_SP(IDX_KS_WEAK_Z, WeakKingZone);
    INIT_BASE_SP(IDX_KS_CHECK_N, SafeKnightCheck);
    INIT_BASE_SP(IDX_KS_CHECK_B, SafeBishopCheck);
    INIT_BASE_SP(IDX_KS_CHECK_R, SafeRookCheck);
    INIT_BASE_SP(IDX_KS_CHECK_Q, SafeQueenCheck);
    INIT_BASE_SP(IDX_KS_QUEENLESS, QueenlessAttack);
    INIT_BASE_SPA(IDX_KS_STORM, KingStorm, 24);
    INIT_BASE_SPA(IDX_KS_SHELTER, KingShelter, 24);
    INIT_BASE_SP(IDX_KS_OFFSET, SafetyOffset);

    INIT_BASE_SPA(IDX_KNIGHT_CLOSED_POS, ClosedPosKnight, 5);
    INIT_BASE_SP(IDX_KNIGHT_SHIELDED, KnightShielded);
    INIT_BASE_SP(IDX_KNIGHT_OUTPOST, KnightOutpost);
    INIT_BASE_SP(IDX_KNIGHT_CENTER_OUTPOST, KnightCenterOutpost);
    INIT_BASE_SP(IDX_KNIGHT_SOLID_OUTPOST, KnightSolidOutpost);

    INIT_BASE_SPA(IDX_BISHOP_PAWNS_COLOR, BishopPawnsSameColor, 7);
    INIT_BASE_SP(IDX_BISHOP_PAIR, BishopPairBonus);
    INIT_BASE_SP(IDX_BISHOP_SHIELDED, BishopShielded);
    INIT_BASE_SP(IDX_BISHOP_LONG_DIAG, BishopLongDiagonal);

    INIT_BASE_SP(IDX_ROOK_SEMIOPEN, RookOnSemiOpenFile);
    INIT_BASE_SP(IDX_ROOK_OPEN, RookOnOpenFile);
    INIT_BASE_SP(IDX_ROOK_BLOCKED, RookOnBlockedFile);
    INIT_BASE_SP(IDX_ROOK_XRAY_QUEEN, RookXrayQueen);

    INIT_BASE_SPA(IDX_MOBILITY_KNIGHT, MobilityN, 9);
    INIT_BASE_SPA(IDX_MOBILITY_BISHOP, MobilityB, 14);
    INIT_BASE_SPA(IDX_MOBILITY_ROOK, MobilityR, 15);
    INIT_BASE_SPA(IDX_MOBILITY_QUEEN, MobilityQ, 28);

    INIT_BASE_SP(IDX_BACKWARD, BackwardPenalty);
    INIT_BASE_SP(IDX_STRAGGLER, StragglerPenalty);
    INIT_BASE_SP(IDX_DOUBLED, DoubledPenalty);
    INIT_BASE_SP(IDX_ISOLATED, IsolatedPenalty);

    INIT_BASE_SP(IDX_PAWN_ATK_MINOR, PawnAttacksMinor);
    INIT_BASE_SP(IDX_PAWN_ATK_ROOK, PawnAttacksRook);
    INIT_BASE_SP(IDX_PAWN_ATK_QUEEN, PawnAttacksQueen);
    INIT_BASE_SP(IDX_MINOR_ATK_ROOK, MinorAttacksRook);
    INIT_BASE_SP(IDX_MINOR_ATK_QUEEN, MinorAttacksQueen);
    INIT_BASE_SP(IDX_ROOK_ATK_QUEEN, RookAttacksQueen);

    extern const scorepair_t PassedBonus[RANK_NB], PhalanxBonus[RANK_NB], DefenderBonus[RANK_NB];

    for (rank_t r = RANK_2; r <= RANK_7; ++r)
    {
        base[IDX_PASSER + r - RANK_2][MIDGAME] = midgame_score(PassedBonus[r]);
        base[IDX_PASSER + r - RANK_2][ENDGAME] = endgame_score(PassedBonus[r]);

        base[IDX_PHALANX + r - RANK_2][MIDGAME] = midgame_score(PhalanxBonus[r]);
        base[IDX_PHALANX + r - RANK_2][ENDGAME] = endgame_score(PhalanxBonus[r]);

        // No pawns can be defenders on the 7th rank

        if (r != RANK_7)
        {
            base[IDX_DEFENDER + r - RANK_2][MIDGAME] = midgame_score(DefenderBonus[r]);
            base[IDX_DEFENDER + r - RANK_2][ENDGAME] = endgame_score(DefenderBonus[r]);
        }
    }

    extern const scorepair_t PP_OurKingProximity[8], PP_TheirKingProximity[8];

    for (int distance = 1; distance <= 7; ++distance)
    {
        base[IDX_PP_OUR_KING_PROX + distance - 1][MIDGAME] =
            midgame_score(PP_OurKingProximity[distance]);
        base[IDX_PP_OUR_KING_PROX + distance - 1][ENDGAME] =
            endgame_score(PP_OurKingProximity[distance]);

        base[IDX_PP_THEIR_KING_PROX + distance - 1][MIDGAME] =
            midgame_score(PP_TheirKingProximity[distance]);
        base[IDX_PP_THEIR_KING_PROX + distance - 1][ENDGAME] =
            endgame_score(PP_TheirKingProximity[distance]);
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
        if (init_tuner_entry(cur, &board)) data->size++;

        if (data->size && data->size % 100000 == 0)
        {
            printf("%u positions loaded\n", (unsigned int)data->size);
            fflush(stdout);
        }
    }
    putchar('\n');
}

bool init_tuner_entry(tune_entry_t *entry, const board_t *board)
{
    entry->staticEval = evaluate(board);
    if (Trace.scaleFactor == 0) return (false);
    if (board->sideToMove == BLACK) entry->staticEval = -entry->staticEval;

    entry->phase = Trace.phase;
    entry->phaseFactors[MIDGAME] = (entry->phase - ENDGAME_COUNT) / (double)(MIDGAME_COUNT - ENDGAME_COUNT);
    entry->phaseFactors[ENDGAME] = 1 - entry->phaseFactors[MIDGAME];

    init_tuner_tuples(entry);

    entry->eval = Trace.eval;
    entry->safety[WHITE] = Trace.safety[WHITE];
    entry->safety[BLACK] = Trace.safety[BLACK];
    entry->scaleFactor = Trace.scaleFactor / 128.0;
    entry->sideToMove = board->sideToMove;
    return (true);
}

bool is_safety_term(int i) { return (i >= IDX_KS_KNIGHT); }

bool is_active(int i)
{
    if (Trace.coeffs[i][WHITE] != Trace.coeffs[i][BLACK]) return (true);
    return (is_safety_term(i) && (Trace.coeffs[i][WHITE] || Trace.coeffs[i][BLACK]));
}

void init_tuner_tuples(tune_entry_t *entry)
{
    int length = 0;
    int tidx = 0;

    for (int i = 0; i < IDX_COUNT; ++i) length += is_active(i);

    entry->tupleCount = length;
    entry->tuples = malloc(sizeof(tune_tuple_t) * length);

    if (entry->tuples == NULL)
    {
        perror("Unable to allocate entry tuples");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < IDX_COUNT; ++i)
        if (is_active(i))
            entry->tuples[tidx++] =
                (tune_tuple_t){i, Trace.coeffs[i][WHITE], Trace.coeffs[i][BLACK]};
}

double compute_optimal_k(const tune_data_t *data)
{
    double start = 0;
    double end = 10;
    double step = 1;
    double cur = start;
    double error;
    double best = static_eval_mse(data, cur);
    double bestK = cur;

    printf("Computing optimal K...\n");
    fflush(stdout);

    for (int i = 0; i < 10; ++i)
    {
        cur = start - step;
        while (cur < end)
        {
            cur += step;
            error = static_eval_mse(data, cur);
            if (error < best)
            {
                best = error;
                bestK = cur;
            }
        }
        printf("Iteration %d/10, K %lf, MSE %lf\n", i + 1, bestK, best);
        fflush(stdout);
        end = bestK + step;
        start = bestK - step;
        step /= 10;
    }
    putchar('\n');
    return (start);
}

double static_eval_mse(const tune_data_t *data, double K)
{
    double total = 0;

#pragma omp parallel shared(total)
    {
#pragma omp for schedule(static, data->size / THREADS) reduction(+ : total)
        for (size_t i = 0; i < data->size; ++i)
            total += pow(data->entries[i].gameResult - sigmoid(K, data->entries[i].staticEval), 2);
    }
    return (total / data->size);
}

double adjusted_eval_mse(const tune_data_t *data, const tp_vector_t delta, double K)
{
    double safetyScores[COLOR_NB][PHASE_NB];
    double result = 0.0;

    for (size_t i = 0; i < data->size; ++i)
        result += pow(data->entries[i].gameResult
                          - sigmoid(K, adjusted_eval(data->entries + i, delta, safetyScores)),
            2);

    return (result / data->size);
}

double adjusted_eval(
    const tune_entry_t *entry, const tp_vector_t delta, double safetyScores[COLOR_NB][PHASE_NB])
{
    double mixed;
    double midgame, endgame, wsafety[PHASE_NB], bsafety[PHASE_NB];
    double mg[2][COLOR_NB] = {0};
    double eg[2][COLOR_NB] = {0};
    double normal[PHASE_NB], safety[PHASE_NB];

    // Save any modifications for MG or EG for each evaluation type.

    for (int i = 0; i < entry->tupleCount; ++i)
    {
        int index = entry->tuples[i].index;
        bool isSafety = is_safety_term(index);

        mg[isSafety][WHITE] += entry->tuples[i].wcoeff * delta[index][MIDGAME];
        mg[isSafety][BLACK] += entry->tuples[i].bcoeff * delta[index][MIDGAME];
        eg[isSafety][WHITE] += entry->tuples[i].wcoeff * delta[index][ENDGAME];
        eg[isSafety][BLACK] += entry->tuples[i].bcoeff * delta[index][ENDGAME];
    }

    // Grab the original non-safety evaluations and add the modified parameters.

    normal[MIDGAME] = (double)midgame_score(entry->eval) + mg[0][WHITE] - mg[0][BLACK];
    normal[ENDGAME] = (double)endgame_score(entry->eval) + eg[0][WHITE] - eg[0][BLACK];

    // Grab the original safety evaluations and add the modified parameters.

    wsafety[MIDGAME] = (double)midgame_score(entry->safety[WHITE]) + mg[1][WHITE];
    wsafety[ENDGAME] = (double)endgame_score(entry->safety[WHITE]) + eg[1][WHITE];
    bsafety[MIDGAME] = (double)midgame_score(entry->safety[BLACK]) + mg[1][BLACK];
    bsafety[ENDGAME] = (double)endgame_score(entry->safety[BLACK]) + eg[1][BLACK];

    // Remove the original safety evaluations from the normal evaluations.

    normal[MIDGAME] -=
        max(0, midgame_score(entry->safety[WHITE])) * midgame_score(entry->safety[WHITE]) / 256
        - max(0, midgame_score(entry->safety[BLACK])) * midgame_score(entry->safety[BLACK]) / 256;
    normal[ENDGAME] -= max(0, endgame_score(entry->safety[WHITE])) / 16
                       - max(0, endgame_score(entry->safety[BLACK])) / 16;

    // Compute the new safety evaluations for each side.

    safety[MIDGAME] = fmax(0, wsafety[MIDGAME]) * wsafety[MIDGAME] / 256.0
                      - fmax(0, bsafety[MIDGAME]) * bsafety[MIDGAME] / 256.0;
    safety[ENDGAME] = fmax(0, wsafety[ENDGAME]) / 16.0 - fmax(0, bsafety[ENDGAME]) / 16.0;

    // Save the safety scores for computing gradients later.

    safetyScores[WHITE][MIDGAME] = wsafety[MIDGAME];
    safetyScores[WHITE][ENDGAME] = wsafety[ENDGAME];
    safetyScores[BLACK][MIDGAME] = bsafety[MIDGAME];
    safetyScores[BLACK][ENDGAME] = bsafety[ENDGAME];

    midgame = normal[MIDGAME] + safety[MIDGAME];
    endgame = normal[ENDGAME] + safety[ENDGAME];

    mixed = midgame * entry->phaseFactors[MIDGAME]
            + endgame * entry->phaseFactors[ENDGAME] * entry->scaleFactor;

    return (mixed);
}

void compute_gradient(
    const tune_data_t *data, tp_vector_t gradient, const tp_vector_t delta, double K, int batchIdx)
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#pragma omp parallel shared(gradient, mutex)
    {
        tp_vector_t local = {};

#pragma omp for schedule(static, (BATCH_SIZE - 1) / THREADS + 1)
        for (int i = 0; i < BATCH_SIZE; ++i)
            update_gradient(data->entries + (size_t)batchIdx * BATCH_SIZE + i, local, delta, K);

        pthread_mutex_lock(&mutex);

        for (int i = 0; i < IDX_COUNT; ++i)
        {
            gradient[i][MIDGAME] += local[i][MIDGAME];
            gradient[i][ENDGAME] += local[i][ENDGAME];
        }

        pthread_mutex_unlock(&mutex);
    }
}

void update_gradient(
    const tune_entry_t *entry, tp_vector_t gradient, const tp_vector_t delta, double K)
{
    double safetyValues[COLOR_NB][PHASE_NB];
    double E = adjusted_eval(entry, delta, safetyValues);
    double S = sigmoid(K, E);
    double X = (entry->gameResult - S) * S * (1 - S);
    double mgBase = X * entry->phaseFactors[MIDGAME];
    double egBase = X * entry->phaseFactors[ENDGAME];

    int firstTermKS = 0;
    int right = entry->tupleCount;

    while (firstTermKS < right)
    {
        int mid = (firstTermKS + right) / 2;

        if (is_safety_term(entry->tuples[mid].index))
            right = mid;
        else
            firstTermKS = mid + 1;
    }

    for (int i = 0; i < firstTermKS; ++i)
    {
        int index = entry->tuples[i].index;
        int8_t wcoeff = entry->tuples[i].wcoeff;
        int8_t bcoeff = entry->tuples[i].bcoeff;

        gradient[index][MIDGAME] += mgBase * (wcoeff - bcoeff);
        gradient[index][ENDGAME] += egBase * (wcoeff - bcoeff) * entry->scaleFactor;
    }

    for (int i = firstTermKS; i < entry->tupleCount; ++i)
    {
        int index = entry->tuples[i].index;
        int8_t wcoeff = entry->tuples[i].wcoeff;
        int8_t bcoeff = entry->tuples[i].bcoeff;

        gradient[index][MIDGAME] += mgBase / 128.0
                                    * (fmax(safetyValues[WHITE][MIDGAME], 0) * wcoeff
                                        - fmax(safetyValues[BLACK][MIDGAME], 0) * bcoeff);
        gradient[index][ENDGAME] += egBase / 16.0 * entry->scaleFactor
                                    * ((safetyValues[WHITE][MIDGAME] > 0.0) * wcoeff
                                        - (safetyValues[BLACK][ENDGAME] > 0.0) * bcoeff);
    }
}

void print_parameters(const tp_vector_t base, const tp_vector_t delta)
{
    printf("\n Parameters:\n");

#define PRINT_SP(idx, val)                                                                       \
    do {                                                                                         \
        printf("scorepair_t %s = SPAIR(%.lf, %.lf);\n", #val,                                    \
            base[idx][MIDGAME] + delta[idx][MIDGAME], base[idx][ENDGAME] + delta[idx][ENDGAME]); \
    } while (0)

#define PRINT_SPA(idx, val, size, pad, lineSplit, prefix)              \
    do {                                                               \
        printf("scorepair_t %s[%d] = {\n    ", #val, size);            \
        for (int i = 0; i < size; ++i)                                 \
            printf(prefix "(%*.lf,%*.lf)%s", pad,                      \
                base[idx + i][MIDGAME] + delta[idx + i][MIDGAME], pad, \
                base[idx + i][ENDGAME] + delta[idx + i][ENDGAME],      \
                (i == size - 1)                    ? "\n"              \
                : (i % lineSplit == lineSplit - 1) ? ",\n    "         \
                                                   : ", ");            \
        puts("};");                                                    \
    } while (0)

    PRINT_SP(IDX_PIECE + 0, PawnValue);
    PRINT_SP(IDX_PIECE + 1, KnightValue);
    PRINT_SP(IDX_PIECE + 2, BishopValue);
    PRINT_SP(IDX_PIECE + 3, RookValue);
    PRINT_SP(IDX_PIECE + 4, QueenValue);

    putchar('\n');

    PRINT_SPA(IDX_PSQT, PawnSQT, 48, 3, 8, "S");

    putchar('\n');

    PRINT_SPA(IDX_PSQT + 48 + 32 * 0, KnightSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 1, BishopSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 2, RookSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 3, QueenSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 4, KingSQT, 32, 4, 4, "S");
    putchar('\n');

    PRINT_SP(IDX_CASTLING, CastlingBonus);
    PRINT_SP(IDX_INITIATIVE, Initiative);
    putchar('\n');

    PRINT_SP(IDX_KS_KNIGHT, KnightWeight);
    PRINT_SP(IDX_KS_BISHOP, BishopWeight);
    PRINT_SP(IDX_KS_ROOK, RookWeight);
    PRINT_SP(IDX_KS_QUEEN, QueenWeight);
    PRINT_SP(IDX_KS_ATTACK, AttackWeight);
    PRINT_SP(IDX_KS_WEAK_Z, WeakKingZone);
    PRINT_SP(IDX_KS_CHECK_N, SafeKnightCheck);
    PRINT_SP(IDX_KS_CHECK_B, SafeBishopCheck);
    PRINT_SP(IDX_KS_CHECK_R, SafeRookCheck);
    PRINT_SP(IDX_KS_CHECK_Q, SafeQueenCheck);
    PRINT_SP(IDX_KS_QUEENLESS, QueenlessAttack);
    putchar('\n');
    PRINT_SPA(IDX_KS_STORM, KingStorm, 24, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_KS_SHELTER, KingShelter, 24, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SP(IDX_KS_OFFSET, SafetyOffset);
    putchar('\n');

    PRINT_SPA(IDX_KNIGHT_CLOSED_POS, ClosedPosKnight, 5, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SP(IDX_KNIGHT_SHIELDED, KnightShielded);
    PRINT_SP(IDX_KNIGHT_OUTPOST, KnightOutpost);
    PRINT_SP(IDX_KNIGHT_CENTER_OUTPOST, KnightCenterOutpost);
    PRINT_SP(IDX_KNIGHT_SOLID_OUTPOST, KnightSolidOutpost);
    putchar('\n');

    PRINT_SPA(IDX_BISHOP_PAWNS_COLOR, BishopPawnsSameColor, 7, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SP(IDX_BISHOP_PAIR, BishopPairBonus);
    PRINT_SP(IDX_BISHOP_SHIELDED, BishopShielded);
    PRINT_SP(IDX_BISHOP_LONG_DIAG, BishopLongDiagonal);
    putchar('\n');

    PRINT_SP(IDX_ROOK_SEMIOPEN, RookOnSemiOpenFile);
    PRINT_SP(IDX_ROOK_OPEN, RookOnOpenFile);
    PRINT_SP(IDX_ROOK_BLOCKED, RookOnBlockedFile);
    PRINT_SP(IDX_ROOK_XRAY_QUEEN, RookXrayQueen);
    putchar('\n');

    PRINT_SPA(IDX_MOBILITY_KNIGHT, MobilityN, 9, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_BISHOP, MobilityB, 14, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_ROOK, MobilityR, 15, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_QUEEN, MobilityQ, 28, 4, 4, "SPAIR");
    putchar('\n');

    PRINT_SP(IDX_BACKWARD, BackwardPenalty);
    PRINT_SP(IDX_STRAGGLER, StragglerPenalty);
    PRINT_SP(IDX_DOUBLED, DoubledPenalty);
    PRINT_SP(IDX_ISOLATED, IsolatedPenalty);
    putchar('\n');

    PRINT_SPA(IDX_PASSER, PassedBonus, 6, 3, 1, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_PHALANX, PhalanxBonus, 6, 3, 1, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_DEFENDER, DefenderBonus, 5, 3, 1, "SPAIR");
    putchar('\n');

    PRINT_SPA(IDX_PP_OUR_KING_PROX, PP_OurKingProximity, 7, 4, 1, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_PP_THEIR_KING_PROX, PP_TheirKingProximity, 7, 4, 1, "SPAIR");
    putchar('\n');

    PRINT_SP(IDX_PAWN_ATK_MINOR, PawnAttacksMinor);
    PRINT_SP(IDX_PAWN_ATK_ROOK, PawnAttacksRook);
    PRINT_SP(IDX_PAWN_ATK_QUEEN, PawnAttacksQueen);
    PRINT_SP(IDX_MINOR_ATK_ROOK, MinorAttacksRook);
    PRINT_SP(IDX_MINOR_ATK_QUEEN, MinorAttacksQueen);
    PRINT_SP(IDX_ROOK_ATK_QUEEN, RookAttacksQueen);
    putchar('\n');
}

double sigmoid(double K, double E) { return (1.0 / (1.0 + exp(-E * K))); }

#endif
