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

#include "tuner.h"
#include "types.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TUNE

static void reset_tp_vector(TpVector *vec)
{
    for (size_t i = 0; i < IDX_COUNT; ++i) vec->v[i][0] = vec->v[i][1] = 0.0;
}

static void adam_init(AdamOptimizer *adam)
{
    reset_tp_vector(&adam->gradient);
    reset_tp_vector(&adam->momentum);
    reset_tp_vector(&adam->velocity);
}

static void init_base_values(TpVector *base)
{
#define INIT_BASE_SP(idx, val)                      \
    do {                                            \
        extern const scorepair_t val;               \
        base->v[idx][MIDGAME] = midgame_score(val); \
        base->v[idx][ENDGAME] = endgame_score(val); \
    } while (0)

#define INIT_BASE_SPA(idx, val, size)                          \
    do {                                                       \
        extern const scorepair_t val[size];                    \
        for (int i = 0; i < size; ++i)                         \
        {                                                      \
            base->v[idx + i][MIDGAME] = midgame_score(val[i]); \
            base->v[idx + i][ENDGAME] = endgame_score(val[i]); \
        }                                                      \
    } while (0)

    base->v[IDX_PIECE + 0][MIDGAME] = PAWN_MG_SCORE;
    base->v[IDX_PIECE + 0][ENDGAME] = PAWN_EG_SCORE;
    base->v[IDX_PIECE + 1][MIDGAME] = KNIGHT_MG_SCORE;
    base->v[IDX_PIECE + 1][ENDGAME] = KNIGHT_EG_SCORE;
    base->v[IDX_PIECE + 2][MIDGAME] = BISHOP_MG_SCORE;
    base->v[IDX_PIECE + 2][ENDGAME] = BISHOP_EG_SCORE;
    base->v[IDX_PIECE + 3][MIDGAME] = ROOK_MG_SCORE;
    base->v[IDX_PIECE + 3][ENDGAME] = ROOK_EG_SCORE;
    base->v[IDX_PIECE + 4][MIDGAME] = QUEEN_MG_SCORE;
    base->v[IDX_PIECE + 4][ENDGAME] = QUEEN_EG_SCORE;

    INIT_BASE_SPA(IDX_PSQT, PawnSQT, 48);
    INIT_BASE_SPA(IDX_PSQT + 48, KnightSQT, 32);
    INIT_BASE_SPA(IDX_PSQT + 80, BishopSQT, 32);
    INIT_BASE_SPA(IDX_PSQT + 112, RookSQT, 32);
    INIT_BASE_SPA(IDX_PSQT + 144, QueenSQT, 32);
    INIT_BASE_SPA(IDX_PSQT + 176, KingSQT, 32);

    INIT_BASE_SP(IDX_INITIATIVE, Initiative);

    INIT_BASE_SP(IDX_KS_KNIGHT, KnightWeight);
    INIT_BASE_SP(IDX_KS_BISHOP, BishopWeight);
    INIT_BASE_SP(IDX_KS_ROOK, RookWeight);
    INIT_BASE_SP(IDX_KS_QUEEN, QueenWeight);
    INIT_BASE_SP(IDX_KS_ATTACK, AttackWeight);
    INIT_BASE_SP(IDX_KS_ASYM_KINGS, AsymmetricKings);
    INIT_BASE_SP(IDX_KS_WEAK_Z, WeakKingZone);
    INIT_BASE_SP(IDX_KS_CHECK_N, SafeKnightCheck);
    INIT_BASE_SP(IDX_KS_CHECK_B, SafeBishopCheck);
    INIT_BASE_SP(IDX_KS_CHECK_R, SafeRookCheck);
    INIT_BASE_SP(IDX_KS_CHECK_Q, SafeQueenCheck);
    INIT_BASE_SP(IDX_KS_UNSAFE_CHECK, UnsafeCheck);
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
    INIT_BASE_SP(IDX_ROOK_TRAPPED, RookTrapped);
    INIT_BASE_SP(IDX_ROOK_BURIED, RookBuried);

    INIT_BASE_SPA(IDX_MOBILITY_KNIGHT, MobilityN, 9);
    INIT_BASE_SPA(IDX_MOBILITY_BISHOP, MobilityB, 14);
    INIT_BASE_SPA(IDX_MOBILITY_ROOK, MobilityR, 15);
    INIT_BASE_SPA(IDX_MOBILITY_QUEEN, MobilityQ, 28);

    INIT_BASE_SP(IDX_BACKWARD, BackwardPenalty);
    INIT_BASE_SP(IDX_STRAGGLER, StragglerPenalty);
    INIT_BASE_SP(IDX_DOUBLED, DoubledPenalty);
    INIT_BASE_SP(IDX_ISOLATED, IsolatedPenalty);

    INIT_BASE_SPA(IDX_PP_OUR_KING_PROX, PP_OurKingProximity, 24);
    INIT_BASE_SPA(IDX_PP_THEIR_KING_PROX, PP_TheirKingProximity, 24);

    INIT_BASE_SP(IDX_PAWN_ATK_MINOR, PawnAttacksMinor);
    INIT_BASE_SP(IDX_PAWN_ATK_ROOK, PawnAttacksRook);
    INIT_BASE_SP(IDX_PAWN_ATK_QUEEN, PawnAttacksQueen);
    INIT_BASE_SP(IDX_MINOR_ATK_ROOK, MinorAttacksRook);
    INIT_BASE_SP(IDX_MINOR_ATK_QUEEN, MinorAttacksQueen);
    INIT_BASE_SP(IDX_ROOK_ATK_QUEEN, RookAttacksQueen);

    extern const scorepair_t PassedBonus[RANK_NB], PhalanxBonus[RANK_NB], DefenderBonus[RANK_NB];

    for (rank_t r = RANK_2; r <= RANK_7; ++r)
    {
        base->v[IDX_PASSER + r - RANK_2][MIDGAME] = midgame_score(PassedBonus[r]);
        base->v[IDX_PASSER + r - RANK_2][ENDGAME] = endgame_score(PassedBonus[r]);

        base->v[IDX_PHALANX + r - RANK_2][MIDGAME] = midgame_score(PhalanxBonus[r]);
        base->v[IDX_PHALANX + r - RANK_2][ENDGAME] = endgame_score(PhalanxBonus[r]);

        // No pawns can be defenders on the 7th rank.
        if (r != RANK_7)
        {
            base->v[IDX_DEFENDER + r - RANK_2][MIDGAME] = midgame_score(DefenderBonus[r]);
            base->v[IDX_DEFENDER + r - RANK_2][ENDGAME] = endgame_score(DefenderBonus[r]);
        }
    }
}

bool is_safety_term(int i) { return i > IDX_KING_SAFETY; }

bool is_active(int i)
{
    if (Trace.coeffs[i][WHITE] != Trace.coeffs[i][BLACK]) return true;
    return is_safety_term(i) && (Trace.coeffs[i][WHITE] || Trace.coeffs[i][BLACK]);
}

void init_tuner_tuples(TuneEntry *entry)
{
    int length = 0;
    int tidx = 0;

    for (int i = 0; i < IDX_COUNT; ++i) length += is_active(i);

    entry->tupleCount = length;
    entry->tuples = malloc(sizeof(TuneTuple) * length);

    if (entry->tuples == NULL)
    {
        perror("Unable to allocate entry tuples");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < IDX_COUNT; ++i)
        if (is_active(i))
            entry->tuples[tidx++] = (TuneTuple){i, Trace.coeffs[i][WHITE], Trace.coeffs[i][BLACK]};
}

bool init_tuner_entry(TuneEntry *entry, const Board *board)
{
    entry->staticEval = evaluate(board);
    if (Trace.scaleFactor == 0) return false;
    if (board->sideToMove == BLACK) entry->staticEval = -entry->staticEval;

    entry->phase = Trace.phase;
    entry->phaseFactors[MIDGAME] =
        (entry->phase - ENDGAME_COUNT) / (double)(MIDGAME_COUNT - ENDGAME_COUNT);
    entry->phaseFactors[ENDGAME] = 1 - entry->phaseFactors[MIDGAME];

    init_tuner_tuples(entry);

    entry->eval = Trace.eval;
    entry->safety[WHITE] = Trace.safety[WHITE];
    entry->safety[BLACK] = Trace.safety[BLACK];
    entry->scaleFactor = Trace.scaleFactor / 256.0;
    entry->sideToMove = board->sideToMove;
    return true;
}

void init_tuner_entries(TuneDataset *data, const char *filename)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
    {
        perror("Unable to open dataset");
        exit(EXIT_FAILURE);
    }

    Board board = {};
    Boardstack stack = {};
    char linebuf[1024];

    while (fgets(linebuf, sizeof(linebuf), f) != NULL)
    {
        if (data->maxSize == data->size)
        {
            data->maxSize += !data->maxSize ? 16 : data->maxSize / 2;
            data->entries = realloc(data->entries, sizeof(TuneEntry) * data->maxSize);

            if (data->entries == NULL)
            {
                perror("Unable to allocate dataset entries");
                exit(EXIT_FAILURE);
            }
        }

        TuneEntry *cur = &data->entries[data->size];

        char *ptr = strrchr(linebuf, ' ');

        *ptr = '\0';

        if (sscanf(ptr + 1, "%hd", &cur->gameScore) == 0)
        {
            fputs("Unable to read game score\n", stdout);
            exit(EXIT_FAILURE);
        }

        ptr = strrchr(linebuf, ' ');

        *ptr = '\0';
        if (sscanf(ptr + 1, "%lf", &cur->gameResult) == 0)
        {
            fputs("Unable to read game result\n", stdout);
            exit(EXIT_FAILURE);
        }

        if (board_from_fen(&board, linebuf, false, &stack) < 0)
        {
            printf("Invalid FEN in dataset: '%s'\n", linebuf);
            exit(EXIT_FAILURE);
        }

        if (init_tuner_entry(cur, &board))
        {
            data->size++;

            if (data->size && data->size % 100000 == 0)
            {
                printf("%u positions loaded\n", (unsigned int)data->size);
                fflush(stdout);
            }
        }
    }

    printf("%u positions loaded\n\n", (unsigned int)data->size);
}

double sigmoid(double k, double eval) { return 1.0 / (1.0 + exp(-eval * k)); }

double static_eval_mse(const TuneDataset *data, double sigmoidK)
{
    double total = 0;

#pragma omp parallel shared(total) num_threads(THREADS)
    {
#pragma omp for schedule(static) reduction(+ : total)
        for (size_t i = 0; i < data->size; ++i)
        {
            const TuneEntry *entry = data->entries + i;

            double result =
                entry->gameResult * (1.0 - LAMBDA) + sigmoid(sigmoidK, entry->gameScore) * LAMBDA;
            total += pow(result - sigmoid(sigmoidK, entry->staticEval), 2);
        }
    }
    return total / data->size;
}

double compute_optimal_k(const TuneDataset *data)
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
    return bestK;
}

size_t first_safety_term(const TuneEntry *entry)
{
    size_t left = 0;
    size_t right = entry->tupleCount;

    while (left < right)
    {
        size_t middle = (left + right) / 2;

        if (is_safety_term(entry->tuples[middle].index))
            right = middle;
        else
            left = middle + 1;
    }

    return left;
}

double adjusted_eval(const TuneEntry *restrict entry, const TpVector *restrict delta,
    TpSplitEval *restrict safetyValues, size_t safetyStart)
{
    double mixed;
    double midgame, endgame, wsafety[PHASE_NB], bsafety[PHASE_NB];
    double linear[PHASE_NB], safety[PHASE_NB];
    double mgLinear = 0.0, egLinear = 0.0;
    double mgSafety[COLOR_NB] = {0.0}, egSafety[COLOR_NB] = {0.0};

    // Save any modifications for MG or EG for each evaluation type.
    for (size_t i = 0; i < safetyStart; ++i)
    {
        const size_t index = entry->tuples[i].index;
        const int8_t diff = entry->tuples[i].wcoeff - entry->tuples[i].bcoeff;

        mgLinear += diff * delta->v[index][MIDGAME];
        egLinear += diff * delta->v[index][ENDGAME];
    }

    for (size_t i = safetyStart; i < entry->tupleCount; ++i)
    {
        const size_t index = entry->tuples[i].index;
        const int8_t wcoeff = entry->tuples[i].wcoeff;
        const int8_t bcoeff = entry->tuples[i].bcoeff;

        mgSafety[WHITE] += wcoeff * delta->v[index][MIDGAME];
        mgSafety[BLACK] += bcoeff * delta->v[index][MIDGAME];
        egSafety[WHITE] += wcoeff * delta->v[index][ENDGAME];
        egSafety[BLACK] += bcoeff * delta->v[index][ENDGAME];
    }

    // Grab the original non-safety evaluations and add the modified parameters.
    linear[MIDGAME] = (double)midgame_score(entry->eval) + mgLinear;
    linear[ENDGAME] = (double)endgame_score(entry->eval) + egLinear;

    // Grab the original safety evaluations and add the modified parameters.
    wsafety[MIDGAME] = (double)midgame_score(entry->safety[WHITE]) + mgSafety[WHITE];
    wsafety[ENDGAME] = (double)endgame_score(entry->safety[WHITE]) + egSafety[WHITE];
    bsafety[MIDGAME] = (double)midgame_score(entry->safety[BLACK]) + mgSafety[BLACK];
    bsafety[ENDGAME] = (double)endgame_score(entry->safety[BLACK]) + egSafety[BLACK];

    // Remove the original safety evaluations from the normal evaluations.
    linear[MIDGAME] -=
        imax(0, midgame_score(entry->safety[WHITE])) * midgame_score(entry->safety[WHITE]) / 256
        - imax(0, midgame_score(entry->safety[BLACK])) * midgame_score(entry->safety[BLACK]) / 256;

    linear[ENDGAME] -= imax(0, endgame_score(entry->safety[WHITE])) / 16
                       - imax(0, endgame_score(entry->safety[BLACK])) / 16;

    // Compute the new safety evaluations for each side.
    safety[MIDGAME] = fmax(0, wsafety[MIDGAME]) * wsafety[MIDGAME] / 256.0
                      - fmax(0, bsafety[MIDGAME]) * bsafety[MIDGAME] / 256.0;
    safety[ENDGAME] = fmax(0, wsafety[ENDGAME]) / 16.0 - fmax(0, bsafety[ENDGAME]) / 16.0;

    // Save the safety scores for computing gradients later.
    safetyValues->v[WHITE][MIDGAME] = wsafety[MIDGAME];
    safetyValues->v[WHITE][ENDGAME] = wsafety[ENDGAME];
    safetyValues->v[BLACK][MIDGAME] = bsafety[MIDGAME];
    safetyValues->v[BLACK][ENDGAME] = bsafety[ENDGAME];

    midgame = linear[MIDGAME] + safety[MIDGAME];
    endgame = linear[ENDGAME] + safety[ENDGAME];

    mixed = midgame * entry->phaseFactors[MIDGAME]
            + endgame * entry->phaseFactors[ENDGAME] * entry->scaleFactor;

    return mixed;
}

double adjusted_eval_mse(
    const TuneDataset *restrict data, const TpVector *restrict delta, double sigmoidK)
{
    TpSplitEval safetyValues;
    double result = 0.0;

    for (size_t i = 0; i < data->size; ++i)
    {
        const TuneEntry *entry = data->entries + i;
        result += pow(entry->gameResult
                          - sigmoid(sigmoidK,
                              adjusted_eval(entry, delta, &safetyValues, first_safety_term(entry))),
            2);
    }

    return result / data->size;
}

void adam_update_gradient(const TuneEntry *entry, TpVector *restrict gradient,
    const TpVector *restrict delta, double sigmoidK)
{
    TpSplitEval safetyValues;
    const size_t safetyStart = first_safety_term(entry);
    const double eval = adjusted_eval(entry, delta, &safetyValues, safetyStart);
    const double sigm = sigmoid(sigmoidK, eval);
    const double error = (entry->gameResult - sigm) * sigm * (1 - sigm);
    const double mgBase = error * entry->phaseFactors[MIDGAME];
    const double egBase = error * entry->phaseFactors[ENDGAME];

    for (size_t i = 0; i < safetyStart; ++i)
    {
        const size_t index = entry->tuples[i].index;
        const int8_t wcoeff = entry->tuples[i].wcoeff;
        const int8_t bcoeff = entry->tuples[i].bcoeff;

        gradient->v[index][MIDGAME] += mgBase * (wcoeff - bcoeff);
        gradient->v[index][ENDGAME] += egBase * (wcoeff - bcoeff) * entry->scaleFactor;
    }

    const double wsafetyMg = fmax(safetyValues.v[WHITE][MIDGAME], 0);
    const double bsafetyMg = fmax(safetyValues.v[BLACK][MIDGAME], 0);
    const double wsafetyEg = safetyValues.v[WHITE][ENDGAME] > 0.0 ? 1.0 : 0.0;
    const double bsafetyEg = safetyValues.v[BLACK][ENDGAME] > 0.0 ? 1.0 : 0.0;

    for (size_t i = safetyStart; i < entry->tupleCount; ++i)
    {
        const size_t index = entry->tuples[i].index;
        const int8_t wcoeff = entry->tuples[i].wcoeff;
        const int8_t bcoeff = entry->tuples[i].bcoeff;

        gradient->v[index][MIDGAME] += mgBase / 128.0 * (wsafetyMg * wcoeff - bsafetyMg * bcoeff);
        gradient->v[index][ENDGAME] +=
            egBase / 16.0 * entry->scaleFactor * (wsafetyEg * wcoeff - bsafetyEg * bcoeff);
    }
}

void adam_compute_gradient(const TuneDataset *restrict dataset, TpVector *restrict gradient,
    const TpVector *restrict delta, double sigmoidK, size_t batchIdx)
{
    const size_t batchOffset = batchIdx * BATCH_SIZE;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#pragma omp parallel shared(gradient, mutex) num_threads(THREADS)
    {
        TpVector local;

        reset_tp_vector(&local);

#pragma omp for schedule(static)
        for (int i = 0; i < BATCH_SIZE; ++i)
            adam_update_gradient(dataset->entries + batchOffset + i, &local, delta, sigmoidK);

        pthread_mutex_lock(&mutex);

        for (int i = 0; i < IDX_COUNT; ++i)
        {
            gradient->v[i][MIDGAME] += local.v[i][MIDGAME];
            gradient->v[i][ENDGAME] += local.v[i][ENDGAME];
        }

        pthread_mutex_unlock(&mutex);
    }
}

void print_parameters(const TpVector *base, const TpVector *delta)
{
    printf("\n Parameters:\n");

#define PRINT_SP(idx, val)                                          \
    do {                                                            \
        printf("const scorepair_t %s = SPAIR(%.lf, %.lf);\n", #val, \
            base->v[idx][MIDGAME] + delta->v[idx][MIDGAME],         \
            base->v[idx][ENDGAME] + delta->v[idx][ENDGAME]);        \
    } while (0)

#define PRINT_SP_NICE(idx, val, pad, nameAlign)                                        \
    do {                                                                               \
        printf("const scorepair_t %-*s = SPAIR(%*.lf,%*.lf);\n", nameAlign, #val, pad, \
            base->v[idx][MIDGAME] + delta->v[idx][MIDGAME], pad,                       \
            base->v[idx][ENDGAME] + delta->v[idx][ENDGAME]);                           \
    } while (0)

#define PRINT_SPA(idx, val, size, pad, lineSplit, prefix)                    \
    do {                                                                     \
        printf("const scorepair_t %s[%d] = {\n    ", #val, size);            \
        for (int i = 0; i < size; ++i)                                       \
            printf(prefix "(%*.lf,%*.lf)%s", pad,                            \
                base->v[idx + i][MIDGAME] + delta->v[idx + i][MIDGAME], pad, \
                base->v[idx + i][ENDGAME] + delta->v[idx + i][ENDGAME],      \
                (i == size - 1)                    ? "\n"                    \
                : (i % lineSplit == lineSplit - 1) ? ",\n    "               \
                                                   : ", ");                  \
        puts("};");                                                          \
    } while (0)

#define PRINT_SPA_PARTIAL(idx, val, size, start, end, pad, prefix)                               \
    do {                                                                                         \
        printf("const scorepair_t %s[%d] = {\n    ", #val, size);                                \
        for (int i = 0; i < size; ++i)                                                           \
        {                                                                                        \
            if (i >= start && i < end)                                                           \
                printf(prefix "(%*.lf,%*.lf)%s", pad,                                            \
                    base->v[idx + i - start][MIDGAME] + delta->v[idx + i - start][MIDGAME], pad, \
                    base->v[idx + i - start][ENDGAME] + delta->v[idx + i - start][ENDGAME],      \
                    (i == size - 1) ? "\n" : ",\n    ");                                         \
            else                                                                                 \
                printf("0%s", (i == size - 1) ? "\n" : ",\n    ");                               \
        }                                                                                        \
        puts("};");                                                                              \
    } while (0)

    // psq_score.h start
    printf("| psq_score.h |\n\n");

    static const char *pieceNames[5] = {"PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN"};

    printf("// Enum for all pieces' midgame and endgame scores\nenum\n{\n");

    for (int phase = MIDGAME; phase <= ENDGAME; ++phase)
        for (piece_t piece = PAWN; piece <= QUEEN; ++piece)
        {
            printf("    %s_%s_SCORE = %.lf,\n", pieceNames[piece - PAWN],
                phase == MIDGAME ? "MG" : "EG",
                base->v[IDX_PIECE + piece - PAWN][phase]
                    + delta->v[IDX_PIECE + piece - PAWN][phase]);

            if (phase == MIDGAME && piece == QUEEN) putchar('\n');
        }

    printf("};\n\n");

    // psq_score.h end
    // psq_score.c start
    printf("| psq_score.c |\n\n");

    printf("// Square-based Pawn scoring for evaluation\n");
    PRINT_SPA(IDX_PSQT, PawnSQT, 48, 3, 8, "S");
    putchar('\n');

    printf("// Square-based piece scoring for evaluation, using a file symmetry\n");
    PRINT_SPA(IDX_PSQT + 48 + 32 * 0, KnightSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 1, BishopSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 2, RookSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 3, QueenSQT, 32, 4, 4, "S");
    putchar('\n');
    PRINT_SPA(IDX_PSQT + 48 + 32 * 4, KingSQT, 32, 4, 4, "S");
    printf("\n\n");

    // psq_score.c end
    // evaluate.c start
    printf("| evaluate.c |\n\n");

    printf("// Special eval terms\n");
    PRINT_SP(IDX_INITIATIVE, Initiative);
    putchar('\n');

    printf("// King Safety eval terms\n");
    PRINT_SP_NICE(IDX_KS_KNIGHT, KnightWeight, 4, 15);
    PRINT_SP_NICE(IDX_KS_BISHOP, BishopWeight, 4, 15);
    PRINT_SP_NICE(IDX_KS_ROOK, RookWeight, 4, 15);
    PRINT_SP_NICE(IDX_KS_QUEEN, QueenWeight, 4, 15);
    PRINT_SP_NICE(IDX_KS_ATTACK, AttackWeight, 4, 15);
    PRINT_SP_NICE(IDX_KS_ASYM_KINGS, AsymmetricKings, 4, 15);
    PRINT_SP_NICE(IDX_KS_WEAK_Z, WeakKingZone, 4, 15);
    PRINT_SP_NICE(IDX_KS_CHECK_N, SafeKnightCheck, 4, 15);
    PRINT_SP_NICE(IDX_KS_CHECK_B, SafeBishopCheck, 4, 15);
    PRINT_SP_NICE(IDX_KS_CHECK_R, SafeRookCheck, 4, 15);
    PRINT_SP_NICE(IDX_KS_CHECK_Q, SafeQueenCheck, 4, 15);
    PRINT_SP_NICE(IDX_KS_UNSAFE_CHECK, UnsafeCheck, 4, 15);
    PRINT_SP_NICE(IDX_KS_QUEENLESS, QueenlessAttack, 4, 15);
    PRINT_SP_NICE(IDX_KS_OFFSET, SafetyOffset, 4, 15);
    putchar('\n');
    printf("// Storm/Shelter indexes:\n");
    printf("// 0-7 - Side\n// 9-15 - Front\n// 16-23 - Center\n");
    PRINT_SPA(IDX_KS_STORM, KingStorm, 24, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_KS_SHELTER, KingShelter, 24, 4, 4, "SPAIR");
    putchar('\n');

    printf("// Knight eval terms\n");
    PRINT_SP_NICE(IDX_KNIGHT_SHIELDED, KnightShielded, 3, 19);
    PRINT_SP_NICE(IDX_KNIGHT_OUTPOST, KnightOutpost, 3, 19);
    PRINT_SP_NICE(IDX_KNIGHT_CENTER_OUTPOST, KnightCenterOutpost, 3, 19);
    PRINT_SP_NICE(IDX_KNIGHT_SOLID_OUTPOST, KnightSolidOutpost, 3, 19);
    putchar('\n');
    PRINT_SPA(IDX_KNIGHT_CLOSED_POS, ClosedPosKnight, 5, 4, 4, "SPAIR");
    putchar('\n');

    printf("// Bishop eval terms\n");
    PRINT_SP_NICE(IDX_BISHOP_PAIR, BishopPairBonus, 3, 18);
    PRINT_SP_NICE(IDX_BISHOP_SHIELDED, BishopShielded, 3, 18);
    PRINT_SP_NICE(IDX_BISHOP_LONG_DIAG, BishopLongDiagonal, 3, 18);
    putchar('\n');
    PRINT_SPA(IDX_BISHOP_PAWNS_COLOR, BishopPawnsSameColor, 7, 4, 4, "SPAIR");
    putchar('\n');

    printf("// Rook eval terms\n");
    PRINT_SP_NICE(IDX_ROOK_SEMIOPEN, RookOnSemiOpenFile, 3, 18);
    PRINT_SP_NICE(IDX_ROOK_OPEN, RookOnOpenFile, 3, 18);
    PRINT_SP_NICE(IDX_ROOK_BLOCKED, RookOnBlockedFile, 3, 18);
    PRINT_SP_NICE(IDX_ROOK_XRAY_QUEEN, RookXrayQueen, 3, 18);
    PRINT_SP_NICE(IDX_ROOK_TRAPPED, RookTrapped, 3, 18);
    PRINT_SP_NICE(IDX_ROOK_BURIED, RookBuried, 3, 18);
    putchar('\n');

    printf("// Mobility eval terms\n");
    PRINT_SPA(IDX_MOBILITY_KNIGHT, MobilityN, 9, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_BISHOP, MobilityB, 14, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_ROOK, MobilityR, 15, 4, 4, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_MOBILITY_QUEEN, MobilityQ, 28, 4, 4, "SPAIR");
    putchar('\n');

    printf("// Threat eval terms\n");
    PRINT_SP_NICE(IDX_PAWN_ATK_MINOR, PawnAttacksMinor, 3, 17);
    PRINT_SP_NICE(IDX_PAWN_ATK_ROOK, PawnAttacksRook, 3, 17);
    PRINT_SP_NICE(IDX_PAWN_ATK_QUEEN, PawnAttacksQueen, 3, 17);
    PRINT_SP_NICE(IDX_MINOR_ATK_ROOK, MinorAttacksRook, 3, 17);
    PRINT_SP_NICE(IDX_MINOR_ATK_QUEEN, MinorAttacksQueen, 3, 17);
    PRINT_SP_NICE(IDX_ROOK_ATK_QUEEN, RookAttacksQueen, 3, 17);
    putchar('\n');

    // evaluate.c end
    // pawns.c start
    printf("| pawns.c |\n\n");

    printf("// Miscellanous bonus for Pawn structures\n");
    PRINT_SP_NICE(IDX_BACKWARD, BackwardPenalty, 3, 16);
    PRINT_SP_NICE(IDX_STRAGGLER, StragglerPenalty, 3, 16);
    PRINT_SP_NICE(IDX_DOUBLED, DoubledPenalty, 3, 16);
    PRINT_SP_NICE(IDX_ISOLATED, IsolatedPenalty, 3, 16);
    putchar('\n');

    printf("// Rank-based bonus for passed Pawns\n");
    PRINT_SPA_PARTIAL(IDX_PASSER, PassedBonus, 8, 1, 7, 3, "SPAIR");
    putchar('\n');
    printf("// Passed Pawn eval terms\n");
    PRINT_SPA(IDX_PP_OUR_KING_PROX, PP_OurKingProximity, 24, 4, 3, "SPAIR");
    putchar('\n');
    PRINT_SPA(IDX_PP_THEIR_KING_PROX, PP_TheirKingProximity, 24, 4, 3, "SPAIR");
    putchar('\n');
    printf("// Rank-based bonus for phalanx structures\n");
    PRINT_SPA_PARTIAL(IDX_PHALANX, PhalanxBonus, 8, 1, 7, 3, "SPAIR");
    putchar('\n');
    printf("// Rank-based bonus for defenders\n");
    PRINT_SPA_PARTIAL(IDX_DEFENDER, DefenderBonus, 8, 1, 6, 3, "SPAIR");
    putchar('\n');
}

static void compute_wdl_eval_mix(TuneDataset *dataset, double sigmoidK)
{
    for (size_t i = 0; i < dataset->size; ++i)
    {
        TuneEntry *entry = dataset->entries + i;

        entry->gameResult =
            entry->gameResult * (1.0 - LAMBDA) + sigmoid(sigmoidK, entry->gameScore) * LAMBDA;
    }
}

static void adam_update_momentum(double *momentum, double gradient)
{
    *momentum = *momentum * 0.9 + gradient * 0.1;
}

static void adam_update_velocity(double *velocity, double gradient)
{
    *velocity = *velocity * 0.999 + gradient * gradient * 0.001;
}

static void adam_update_delta(double *delta, double momentum, double velocity, double lr)
{
    *delta += momentum * lr / sqrt(1e-8 + velocity);
}

static void adam_update_values(
    AdamOptimizer *restrict adam, TpVector *restrict delta, double sigmoidK, double learningRate)
{
    const double scale = (sigmoidK * 2.0 / BATCH_SIZE);

    for (size_t i = 0; i < IDX_COUNT; ++i)
    {
        double mgGrad = adam->gradient.v[i][MIDGAME] * scale;
        double egGrad = adam->gradient.v[i][ENDGAME] * scale;

        adam_update_momentum(&adam->momentum.v[i][MIDGAME], mgGrad);
        adam_update_momentum(&adam->momentum.v[i][ENDGAME], egGrad);

        adam_update_velocity(&adam->velocity.v[i][MIDGAME], mgGrad);
        adam_update_velocity(&adam->velocity.v[i][ENDGAME], egGrad);
    }

    for (size_t i = 0; i < IDX_COUNT; ++i)
    {
        adam_update_delta(&delta->v[i][MIDGAME], adam->momentum.v[i][MIDGAME],
            adam->velocity.v[i][MIDGAME], learningRate);
        adam_update_delta(&delta->v[i][ENDGAME], adam->momentum.v[i][ENDGAME],
            adam->velocity.v[i][ENDGAME], learningRate);
    }

    reset_tp_vector(&adam->gradient);
}

static void adam_next_epoch(AdamOptimizer *restrict adam, const TuneDataset *restrict dataset,
    TpVector *restrict delta, double sigmoidK, double learningRate)
{
    const size_t batchCount = dataset->size / BATCH_SIZE;

    for (size_t batchIdx = 0; batchIdx < batchCount; ++batchIdx)
    {
        adam_compute_gradient(dataset, &adam->gradient, delta, sigmoidK, batchIdx);
        adam_update_values(adam, delta, sigmoidK, learningRate);
    }
}

#endif

void start_tuning_session(const char *filename)
{
#ifdef TUNE
    AdamOptimizer adam;
    TpVector delta, base;
    TuneDataset dataset = {};

    reset_tp_vector(&delta);
    reset_tp_vector(&base);
    adam_init(&adam);

    init_base_values(&base);
    init_tuner_entries(&dataset, filename);

    const double sigmoidK = compute_optimal_k(&dataset);
    double learningRate = LEARNING_RATE;
    double lastLoss = 0.0;

    compute_wdl_eval_mix(&dataset, sigmoidK);

    for (size_t iter = 0; iter < ITERS; ++iter)
    {
        adam_next_epoch(&adam, &dataset, &delta, sigmoidK, learningRate);

        const double currentLoss = adjusted_eval_mse(&dataset, &delta, sigmoidK);
        const bool earlyStop = iter > 0 && lastLoss - currentLoss < 1e-8;

        printf("Iteration [%u], Loss [%.7lf]\n", (unsigned int)iter, currentLoss);

        if (iter % LR_DROP_ITERS == LR_DROP_ITERS - 1) learningRate /= LR_DROP_VALUE;

        if (iter % 50 == 49 || iter == ITERS - 1 || earlyStop) print_parameters(&base, &delta);

        fflush(stdout);

        if (earlyStop) break;

        lastLoss = currentLoss;
    }

    for (size_t i = 0; i < dataset.size; ++i) free(dataset.entries[i].tuples);
    free(dataset.entries);
#else
    (void)filename;
#endif
}
