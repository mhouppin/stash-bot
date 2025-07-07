# Contributing to Stash

Welcome to yet again another engine project! This document outlines the
guidelines and steps to follow when making contributions to Stash, as well as
documenting some parts of the code used for tuning the search and evaluation.

## Search and eval tweaks

  All search and eval tweaks should go through a STC test and LTC test before a
  potential merge on master. The LTC test will be the deciding factor on
  whether a branch will be merged or not (meaning that a patch that fails at STC
  but passes at LTC can be merged, while the inverse is not true).

  This rule also applies to simplifications, with the only difference being that
  both tests are to be run with simplification bounds.

  Given that the main OpenBench instance tends to saturate easily if a few LTC
  tests are currently running, it is generally good practice to run speculative
  LTCs that didn't go through a green STC at a reduced throughput (250-500).

## QOL and speedup patches

  For QOL and speedup patches, you can sanity check/verify the speed changes
  by using [this script](https://github.com/TerjeKir/EngineTests/blob/master/speedup.py).

## SPSA tuning runs

  If you decide to do SPSA tuning runs on the main OpenBench instance, for the
  same reasons as above, avoid using more than 300k iterations for STC and 100k
  iterations for LTC, unless you use a reduced throughput. Stash has a few
  utilities to help with tuning parameters [here](https://github.com/mhouppin/stash-bot/blob/master/src/include/option.h),
  where you can put a global variable somewhere in the code and tune it by
  adding a `TUNE_X(MyGlobal, minValue, maxValue)` in the `uci_init_options()`
  function. You can then grab the formatted OpenBench tuning input by using
  the `t` command in Stash.

## Evaluation tuning

  For new evaluation parameters, the protocol to tune them is to:
  - Add a new enumeration value in [evaluate.h](https://github.com/mhouppin/stash-bot/blob/master/src/include/evaluate.h);
  - Add a `trace_add()` in the evaluation code to track when the new parameter
    is activated;
  - Add a `TUNE_ADD_SCOREPAIR()` or a `TUNE_ADD_SP_ARRAY()` in the
    `init_disp_sequence_and_base_values()` function at the bottom of [tuner.c](https://github.com/mhouppin/stash-bot/blob/master/src/sources/tuner.c).

  > [!WARNING]
  > All of this is only required if you're planning to generate a new dataset
    for experimenting. You can just contact me directly on Stockfish's Discord
    server if you want to do a tuning run with the current dataset.

  Stash currently uses a mix of WDL and search score for tuning its evaluation.
  The protocol is:
  - Generate 1M selfplay games at 16knpm with fastchess/cutechess, while keeping
    PGN comments with score information (this is done about once every release,
    because to the CPU time it takes);
  - Extract ~10M FENs from those games using [this script](https://github.com/mhouppin/stash_tools/blob/main/scripts/extract.py).
    > Outdated information, I still need to clean up the current tool for
      generating the dataset from the PGN file and publish it.

  For starting a tuning session, you'll need to:
  - Change some of the tuner settings at the top of [tuner.c](https://github.com/mhouppin/stash-bot/blob/master/src/sources/tuner.c)
    (for example, if you want to run the tuning session on several threads);
  - Compiling Stash in tuning mode by running
    `make CFLAGS="-O3 -fopenmp -flto -DTUNE -DNDEBUG" NATIVE=yes`;
  - Running Stash with the scored dataset file in argument;
  - Waiting for the tuning run to finish and copying all the printed eval parts
    back into the codebase.
  > [!NOTE]
  > To be sure that your new eval parameter is tuned correctly, you can set the
    number of iterations to 50 to make a quick test run to ensure that the
    parameter is correctly displayed during the tuning run, and that its value
    effectively changes over time. You can set back the iteration count to 1000
    afterwards.

## General rules for all patches

  Unless your PR does not modify the source code in `src/` (comment fixes,
  documentation updates, etc.), you need to increment the minor version number
  present at the top of `uci.c`.
  Run `clang-format -style=file` if your changes might not be formatted
  correctly.
  If you did SPSA/SPRT runs for this PR on the main OpenBench instance, link to
  all tests relating to the patch (even if a STC test might have failed red).
