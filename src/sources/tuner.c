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

#include "tuner.h"

void start_tuning_session(const char *filename)
{
    tp_array_t methods = {};
    tp_vector_t delta = {}, base = {};
    double K, mse, lr = LEARNING_RATE;
    tune_data_t data = {};

    init_base_values(base);
    init_methods(methods);
    init_tuner_entries(&data, filename);
    K = compute_optimal_k(&data);

    for (int iter = 0; iter < ITERS; ++iter)
    {
        for (int batchIdx = 0; batchIdx < data.size / BATCH_SIZE; ++batchIdx)
        {
            tp_vector_t gradient = {};
            compute_gradient(&data, gradient, delta, methods, K, batchIdx);

            const double scale = (K * 2.0 / BATCH_SIZE) * lr;

            for (int i = 0; i < IDX_COUNT; ++i)
            {
                delta[i][MIDGAME] += gradient[i][MIDGAME] * scale;
                delta[i][ENDGAME] += gradient[i][ENDGAME] * scale;
            }
        }

        mse = adjusted_eval_mse(&data, delta, methods, K);
        printf("Iteration [%d], MSE [%g], LR [%g]\n", iter, mse, lr);

        if (iter % LR_DROP_ITERS == LR_DROP_ITERS - 1)
            lr /= LR_DROP_VALUE;
        if (iter % 50 == 49)
            print_parameters(base, delta);
    }

    for (size_t i = 0; i < data.size; ++i)
        free(data.entries[i].tuples);
    free(data);
}