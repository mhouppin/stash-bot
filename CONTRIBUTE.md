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
  adding a `TUNE_X(MyGlobal, minValue, maxValue)` in the `uci_loop()` function.

## Evaluation tuning

  For new evaluation parameters, the protocol to tune them is to:
  - Add a new enumeration value in [evaluate.h](https://github.com/mhouppin/stash-bot/blob/master/src/include/evaluate.h);
  - Add a `TRACE_ADD()` in the evaluation code to track when the new parameter
    is activated;
  - Add a `INIT_BASE_SP[A]()` in the `init_base_values()` function in tune.c
    referencing the enumeration value and the evaluation constant;
  - Add a `PRINT_SP[A]()` in the `print_parameters()` function in tune.c
    to print the tuned value of the constant during tuning runs.

  Stash currently uses a mix of WDL and search score for tuning its evaluation.
  The protocol is:
  - Generate 1M selfplay games at 1+0.01, and keep minimal PGNs (this is done
    about once every release, due to the CPU time it takes to perform);
  - Extract ~10M FENs from those games using [this script](https://github.com/mhouppin/stash_tools/blob/main/scripts/extract.py);
  - Scoring the FENs with a search score using [this crate](https://github.com/mhouppin/stash_tools/tree/main/stash_scoring)
    (the current scoring parameters for Stash are
    `--depth 7 --nodes 20000 --config NormalizeScore=false`);
  - Compiling Stash in tuning mode by running
    `CFLAGS="-fopenmp -DTUNE" make re native=yes`;
  - Running Stash with the scored dataset file in argument;
  - Waiting for the tuning run to finish and copying all the printed eval parts
    back into the codebase.

## General rules for all patches

  Unless your PR does not modify a single file in `src/`, you need to increment
  the minor version number present at the top of `uci.c`.
  Run `clang-format -style=file` if your changes might not be formatted
  correctly.
  If you did SPSA/SPRT runs for this PR on the main OpenBench instance, link to
  all tests relating to the patch (even if a STC test might have failed red).
