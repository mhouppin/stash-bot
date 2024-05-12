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

#ifndef TUNER_H
#define TUNER_H

#include "evaluate.h"
#include <stddef.h>

#ifdef TUNE

#define LAMBDA 0.2
#define THREADS 3
#define ITERS 10000
#define LEARNING_RATE 0.001
#define LR_DROP_ITERS 10000
#define LR_DROP_VALUE 1.0
#define BATCH_SIZE 4096

typedef struct _TuneTuple
{
    uint16_t index;
    int8_t wcoeff;
    int8_t bcoeff;
} TuneTuple;

typedef struct _TuneEntry
{
    score_t staticEval;
    int phase;
    color_t sideToMove;
    scorepair_t eval;
    scorepair_t safety[COLOR_NB];
    double gameResult;
    score_t gameScore;
    double scaleFactor;
    double phaseFactors[PHASE_NB];
    TuneTuple *tuples;
    size_t tupleCount;
} TuneEntry;

typedef struct _TuneDataset
{
    TuneEntry *entries;
    size_t size;
    size_t maxSize;
} TuneDataset;

typedef struct _TpVector
{
    double v[IDX_COUNT][2];
} TpVector;

typedef struct _AdamOptimizer
{
    TpVector gradient;
    TpVector momentum;
    TpVector velocity;
} AdamOptimizer;

typedef struct _TpSplitEval
{
    double v[2][PHASE_NB];
} TpSplitEval;

#endif

void start_tuning_session(const char *filename);

#endif
