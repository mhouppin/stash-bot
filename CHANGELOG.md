
## v31.0 (2021-06-26)

### Regression test

- LTC:
  ```
  ELO   | 61.64 +- 7.50 (95%)
  SPRT  | 40.0+0.40s Threads=1 Hash=64MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 1.00]
  Games | N: 3736 W: 1176 L: 520 D: 2040
  ```

### Fixed (3 changes)

- Fixed issues with benchmark containing commas for CI.
- Fixed some odd behaviors on fixed depth/infinite search with SMP + mate
  pruning on root nodes. (This is technically a functional change, but since
  it only triggers in mate-in-one positions, no regression tests were made.)
- Fixed a dead store of the variable 'piece' in undo_move() function.

### Performance (11 changes)

- Added AdaGrad tuning to the code base and retuned all eval parameters. To use
  Stash in tuning mode, compile with `'CFLAGS="-DTUNE -fopenmp" make ...'`. You
  then can do `./stash-bot dataset_file` to launch the tuning session. The
  expected format for the dataset is `FEN OUTCOME`, with the outcome being 1.0
  for White wins, 0.5 for draws, and 0.0 for Black wins. (Note that it is
  possible to use more fine-grained values like 0.7 and such, but it hasn't
  been tested yet.)
  ```
  ELO   | 9.47 +- 6.20 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6496 W: 1840 L: 1663 D: 2993
  ```
  The tuner was later changed to include a validation loss and have a better
  output of tuned values.

- Rewrote the King Safety code from scratch, now including the total attack
  count and the potential safe checks on the enemy King. Also changed the KS
  scaling to be quadratic in middlegame, and linear in endgame.
  ```
  ELO   | 4.24 +- 3.38 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 19736 W: 4941 L: 4700 D: 10095
  ```
  Weak and safe squares were redefined in the next test, pushing the total gain
  to +13 Elo.
  ```
  ELO   | 9.69 +- 5.90 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.08 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6064 W: 1466 L: 1297 D: 3301
  ```

- Allowed capture/promotion moves to be reduced in LMR by one ply.
  ```
  ELO   | 4.88 +- 3.77 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 15174 W: 3640 L: 3427 D: 8107
  ```

- Forced LMR depth to be > 0 to avoid dropping into qsearch.
  ```
  ELO   | 3.14 +- 2.51 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 33842 W: 7925 L: 7619 D: 18298
  ```

- Added ponder support to the codebase.

- Raised SEE Pruning max depth to 5.
  ```
  ELO   | 7.18 +- 4.95 (95%)
  SPRT  | 10.0+0.10s Threads=1 Hash=16MB
  LLR   | 2.99 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 8612 W: 2055 L: 1877 D: 4680
  ```

- Disabled all pruning code when in check and removed eval computing. The logic
  behind that is that King Safety values get pretty wild for positions with
  check, because if the piece attacking the King is a slider, it may have a lot
  of other possible safe checks along the King line. This test was done in two
  commits, the first one removing computations from search:
  ```
  ELO   | 2.36 +- 1.66 (95%)
  SPRT  | 10.0+0.10s Threads=1 Hash=16MB
  LLR   | 3.01 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 75936 W: 17338 L: 16822 D: 41776
  ```
  and the second one, done later, removing computations from qsearch:
  ```
  ELO   | 4.25 +- 3.35 (95%)
  SPRT  | 8.0+0.08s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 17976 W: 4038 L: 3818 D: 10120
  ```

- Allowed entries with lower depth to conditionally replace other ones in TT.
  Credits to Jay Honnold (Berserk's author) for this patch.
  ```
  ELO   | 7.49 +- 5.03 (95%)
  SPRT  | 10.0+0.10s Threads=1 Hash=16MB
  LLR   | 3.03 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 8160 W: 1909 L: 1733 D: 4518

  ELO   | 28.87 +- 11.52 (95%)
  SPRT  | 5.0+0.05s Threads=8 Hash=64MB
  LLR   | 2.95 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 1520 W: 393 L: 267 D: 860
  ```
  A later patch that failed yellow was pushed to make Stash's behavior much
  more stable when reaching positions with lots of fail-highs (such as mate
  positions).
  ```
  ELO   | 0.76 +- 1.58 (95%)
  SPRT  | 40.0+0.40s Threads=1 Hash=64MB
  LLR   | -2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 64400 W: 11243 L: 11102 D: 42055
  ```

- Changed Razoring method so that it only triggers at depth 1.
  ```
  ELO   | 8.05 +- 5.24 (95%)
  SPRT  | 8.0+0.08s Threads=1 Hash=16MB
  LLR   | 3.04 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 7424 W: 1721 L: 1549 D: 4154
  ```

- Add draw specifications for 5-man endgames with an extra pawn on one side,
  such as KRPvKR, KBPvKN and such. (KQPvKQ isn't supported yet.)
  ```
  ELO   | 3.29 +- 2.65 (95%)
  SPRT  | 8.0+0.08s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 28640 W: 6349 L: 6078 D: 16213
  ```

- Added bonuses for connected Pawns (Phalanx and Defender) in Pawn eval.
  ```
  ELO   | 25.38 +- 10.40 (95%)
  SPRT  | 8.0+0.08s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 2002 W: 540 L: 394 D: 1068

  ELO   | 18.57 +- 8.45 (95%)
  SPRT  | 40.0+0.40s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 2640 W: 608 L: 467 D: 1565
  ```

### Changed (3 changes)

- Allowed qsearch to differentiate between PV and non-PV nodes. This also
  permits querying the PV from qsearch.
  ```
  ELO   | 1.29 +- 3.16 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 20200 W: 4433 L: 4358 D: 11409
  ```

- Removed the condition that used to disable aspiration windows with high
  scores. (Latest TT patches made Stash much more competitive at finding mates
  without this condition. This is technically  a functional change, but no
  tests were run because the value is above the resign threshold used for test
  games.)

- Added a script for generating opening books in PGN format to the utils/
  folder, and changed the FEN extractor to do a shallow search before adding
  FENs to the dataset.

## v30.0 (2021-04-30)

### Regression test

- LTC:
  ```
  ELO   | 51.14 +- 4.40 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 0.50]
  Games | N: 8472 W: 2144 L: 906 D: 5422
  ```

### Fixed (1 change)

- Fixed continuation history indexing (was looking for the piece on the 'to'
  square instead of the 'from' square).
  ```
  ELO   | 6.33 +- 5.18 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.97 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 6040 W: 1115 L: 1005 D: 3920
  ```

### Removed (2 changes)

- Removed Knight pair and Rook pair bonuses from the evaluation function.
  ```
  ELO   | 2.08 +- 3.73 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 14867 W: 3370 L: 3281 D: 8216
  ```

- Removed Internal Iterative Reductions from the search.
  ```
  ELO   | 0.31 +- 2.48 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 34064 W: 7711 L: 7681 D: 18672

  ELO   | 3.53 +- 4.08 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 9856 W: 1795 L: 1695 D: 6366
  ```

### Performance (14 changes)

- Made the continuation history additionally use our previous move, and
  changed the butterfly history indexing from piece to color-only.
  ```
  ELO   | 3.58 +- 2.85 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.02 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 25904 W: 6041 L: 5774 D: 14089

  ELO   | 3.04 +- 2.44 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 27000 W: 4796 L: 4560 D: 17644
  ```

- Disabled aspiration windows for very high scores to allow Stash to find
  mates faster.
  ```
  ELO   | 1.57 +- 3.91 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [-5.00, 0.00]
  Games | N: 13924 W: 3228 L: 3165 D: 7531
  ```

- Made the Late Move Reductions start from move 4 instead of 5.
  ```
  ELO   | 2.59 +- 1.95 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 54686 W: 12535 L: 12127 D: 30024

  ELO   | 2.55 +- 2.04 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 38850 W: 6912 L: 6627 D: 25311
  ```
  Changed later to start from move 5 at the root, and 3 otherwise.
  ```
  ELO   | 6.19 +- 4.50 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 10448 W: 2482 L: 2296 D: 5670

  ELO   | 2.45 +- 1.93 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.99 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 43024 W: 7607 L: 7304 D: 28113
  ```

- Made quiescence search use the TT search score in place of the static eval
  if the TT bounds suggest a more accurate result.
  ```
  ELO   | 8.99 +- 5.73 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6336 W: 1507 L: 1343 D: 3486
  ```

- Changed Killer move array to update as a queue instead of a stack (meaning old
  killer moves get replaced by newer ones).
  ```
  ELO   | 5.71 +- 4.23 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 11552 W: 2677 L: 2487 D: 6388
  ```

- Changed piece mobility detection to account for pins.
  ```
  ELO   | 6.15 +- 4.50 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 10623 W: 2569 L: 2381 D: 5673
  ```

- Added Futility Pruning on parent nodes (meaning checking if a move is
  unlikely to improve alpha before playing it).
  ```
  ELO   | 2.08 +- 3.73 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 14867 W: 3370 L: 3281 D: 8216

  ELO   | 6.70 +- 4.41 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 8198 W: 1495 L: 1337 D: 5366
  ```

- Added endgame functions analyzing very specific material configurations
  (like KQvKP or KRvKN).
  ```
  ELO   | 3.76 +- 3.00 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 23448 W: 5474 L: 5220 D: 12754

  ELO   | 3.88 +- 3.11 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 21848 W: 5115 L: 4871 D: 11862
  ```

- Re-enabled check extensions that were disabled after a messed up scope.
  ```
  ELO   | 9.34 +- 6.83 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 4504 W: 1084 L: 963 D: 2457
  ```

- Replaced square-to-bitboard conversion by using a lookup table instead of
  always computing the shift.
  ```
  ELO   | 4.98 +- 3.79 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.02 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 14660 W: 3439 L: 3229 D: 7992
  ```

- Made the KXK specialization more tolerant, changed some endgame scaling
  factors based on pawn count.
  ```
  ELO   | 9.09 +- 5.73 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6308 W: 1495 L: 1330 D: 3483
  ```

- Made Late Move Pruning more aggressive for positions where the eval doesn't
  improve.
  ```
  ELO   | 16.20 +- 8.08 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.99 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 3240 W: 815 L: 664 D: 1761
  ```

- Increased history score granularity (from 1024 to 8192).
  ```
  ELO   | 5.63 +- 4.17 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 11848 W: 2735 L: 2543 D: 6570
  ```

- Increased Late Move Pruning max depth (from 3 to 5).
  ```
  ELO   | 6.73 +- 4.71 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 9184 W: 2111 L: 1933 D: 5140
  ```

### Changed (1 change)

- Very large refactor of the code, including extensive variable renaming. As a
  side result the compilation time halved on some systems.
  ```
  ELO   | 4.48 +- 8.52 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.01 (-2.94, 2.94) [-10.00, 0.00]
  Games | N: 2944 W: 698 L: 660 D: 1586
  ```

## v29.0 (2021-03-05)

### Regression test

- LTC:
  ```
  ELO   | 54.19 +- 10.27 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.97 (-2.94, 2.94) [45.00, 50.00]
  Games | N: 1590 W: 414 L: 168 D: 1008
  ```

### Fixed (2 changes)

- Fixed memory leaks when using score pairs in UCI options.
- Fixed some potential timeouts during CI due to singular extensions on the
  'depth 20' test by adding a 'movetime' limit.

### Performance (4 changes)

- Added staged move generation to the movepicker, and placed bad captures
  to the end of the move list.
  ```
  ELO   | 17.86 +- 8.64 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.04 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 3018 W: 810 L: 655 D: 1553
  ```

- Replaced insertion in place_top_move() by swapping, effectively moving less
  elements when retrieving the next move to analyze.
  ```
  ELO   | 9.18 +- 5.87 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6436 W: 1628 L: 1458 D: 3350
  ```

- Added Static Exchange Evaluation Pruning, and stopped pruning capture moves
  on Late Move Pruning.
  ```
  ELO   | 9.94 +- 6.21 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 5874 W: 1523 L: 1355 D: 2996

  ELO   | 3.95 +- 3.07 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 18098 W: 3442 L: 3236 D: 11420
  ```

- Replaced Late Move Pruning threshold (from quiet move count to move count).
  ```
  ELO   | 18.75 +- 8.76 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.01 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 2782 W: 716 L: 566 D: 1500

  ELO   | 24.52 +- 9.38 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 3.01 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 1930 W: 422 L: 286 D: 1222
  ```

### Changed (2 changes)

- Added some helper macros to facilitate UCI option structure usage when
  running SPSA tests.
- Removed 'mate score' handling in time management to allow mates to be
  correctly solved during search.

## v28.0 (2021-02-23)

### Regression test

- LTC:
  ```
  ELO   | 55.80 +- 10.33 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.98 (-2.94, 2.94) [45.00, 50.00]
  Games | N: 1972 W: 604 L: 290 D: 1078
  ```

### Fixed (3 changes)

- Fixed the multi-job support for the command `make re -j`.
- Fixed the compilation issues for Windows.
- Fixed the center files' bonus detection for Knight Outposts.
  ```
  ELO   | 0.95 +- 3.09 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 23019 W: 5513 L: 5450 D: 12056
  ```

### Performance (12 changes)

- Simplified TT saving by removing redundant checks.
  ```
  ELO   | 5.30 +- 5.48 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 7472 W: 1869 L: 1755 D: 3848
  ```

- Simplified material value updates, added a condition to trigger King Safety
  code only when needed, simplified OCB check.
  ```
  ELO   | 3.50 +- 2.77 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 4.00]
  Games | N: 28704 W: 6976 L: 6687 D: 15041
  ```

- Added Knight Outposts and Shielded minor pieces as eval bonuses.
  ```
  ELO   | 10.16 +- 6.34 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.02 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 5984 W: 1644 L: 1469 D: 2871
  ```

- Enlarged the Pawn table cache, resetting it only when 'ucinewgame' is sent,
  and saved the Pawn attack span in it for quickly finding outposts.
  ```
  ELO   | 8.26 +- 6.57 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 5008 W: 1230 L: 1111 D: 2667
  ```

- Removed the bonus for Far Outposts in the eval.
  ```
  ELO   | 2.36 +- 1.60 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.03 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 85712 W: 20551 L: 19968 D: 45193
  ```

- Increased the history scores' granularity.
  ```
  ELO   | 3.73 +- 3.00 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 24192 W: 5810 L: 5550 D: 12832
  ```

- Tuned the Initiative term, along with the Shielded and Outposts terms.
  ```
  ELO   | 8.14 +- 5.33 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.12 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 7856 W: 1983 L: 1799 D: 4074
  ```

- Added back candidate passers to the pawn evaluation.
  ```
  ELO   | 6.87 +- 4.85 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 9459 W: 2365 L: 2178 D: 4916
  ```

- Changed the condition to trigger Late Move Reductions from "not in check"
  to "not a capture or promotion".
  ```
  ELO   | 6.89 +- 4.88 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 9280 W: 2304 L: 2120 D: 4856

  ELO   | 4.99 +- 3.74 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 12944 W: 2631 L: 2445 D: 7868
  ```

- Added a function for KXK endgames (including the infamous KBNK situation),
  and disabled Null Move Pruning when the side to move has no pieces left.
  ```
  ELO   | 2.61 +- 1.94 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 57580 W: 13719 L: 13287 D: 30574

  ELO   | 4.04 +- 3.16 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 17824 W: 3528 L: 3321 D: 10975
  ```

- Adjusted Late Move Reductions' value based on the move history.
  ```
  ELO   | 3.43 +- 2.76 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 28864 W: 7010 L: 6725 D: 15129

  ELO   | 4.72 +- 3.55 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.97 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 13846 W: 2707 L: 2519 D: 8620
  ```

- Added Singular Extensions to the search.
  ```
  ELO   | 5.91 +- 4.38 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 11280 W: 2732 L: 2540 D: 6008

  ELO   | 13.50 +- 6.87 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 3708 W: 773 L: 629 D: 2306
  ```

### Changed (5 changes)

- Changed seeding to accelerate magic bb generation.
- Changed a lot of function prototypes to make the code cleaner.
- Replaced the color-specialized functions in movegen by generic ones.
- Moved the code used to print the PV string to a new function.
- Merged the root search function with the standard search.
  ```
  ELO   | -2.38 +- 2.45 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | -2.97 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 36119 W: 8354 L: 8601 D: 19164
  ```

## v27.0 (2021-01-14)

### Regression test

- LTC:
  ```
  ELO   | 76.51 +- 14.31 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 3.01 (-2.94, 2.94) [45.00, 50.00]
  Games | N: 992 W: 325 L: 110 D: 557
  ```

### Removed (1 change)

- Removed scaling for KPK and pawnful endgames.
  ```
  ELO   | -1.73 +- 1.73 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | -2.96 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 76000 W: 18423 L: 18802 D: 38775
  ```

### Performance (4 changes)

- Improved the mobility zone by excluding rammed pawns and pawns on low ranks.
  ```
  ELO   | 19.95 +- 9.63 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 2720 W: 817 L: 661 D: 1242
  ```

- Added a countermove history.
  ```
  ELO   | 6.07 +- 4.51 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 11504 W: 3004 L: 2803 D: 5697
  ```

- Added a continuation history.
  ```
  ELO   | 6.06 +- 4.51 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 11232 W: 2873 L: 2677 D: 5682
  ```

- Stopped searching bad captures in Quiescence Search, except if best\_value
  suggests that we're getting mated. (negative-SEE pruning)
  ```
  ELO   | 67.39 +- 18.03 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 736 W: 257 L: 116 D: 363
  ```

### Changed (4 changes)

- Merged some files with very small functions (like the UCI command handlers).
- Merged the piece functions in movegen to a single generic one.
  ```
  ELO   | 1.33 +- 3.44 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.95 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 19360 W: 4831 L: 4757 D: 9772
  ```

- Added support for large transposition tables, currently up to 32 TB of memory.
  ```
  ELO   | -0.63 +- 1.54 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.98 (-2.94, 2.94) [-4.00, 1.00]
  Games | N: 96928 W: 24036 L: 24211 D: 48681
  ```

- Changed Makefile to include x86-64-modern compilation flags by default.

## v26.0

### Regression test

- LTC:
  ```
  ELO   | 68.36 +- 12.97 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [45.00, 50.00]
  Games | N: 1184 W: 370 L: 140 D: 674
  ```

### Fixed (2 changes)

- Fixed a stack overflow on some systems when the search got too deep. (As
  a result each worker uses a minimum of 8 MB for its stack.)

- Fixed an array overflow in King Safety when the opponent has exactly 8 pieces
  attacking the King.

### Performance (4 changes)

- Changed some eval terms' positions (like the piece pair bonus/malus in the
  corresponding piece eval function) to gain some speed.
  ```
  ELO   | 2.59 +- 1.72 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.99 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 88480 W: 25262 L: 24602 D: 38616
  ```

- Changed the Initiative static value to a dynamic one, based on all pieces
  that are threatened by lower-valued ones. The final score is given like this:
  ```
  stm_iv = !in_check + stm_threats;
  opp_iv = opp_threats;
  bonus = Initiative * (stm_iv * stm_iv - opp_iv * opp_iv);
  ```
  so that having attacks on the opponent's pieces is rewarded even more if we
  are the side to move.
  ```
  ELO   | 10.13 +- 6.50 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.99 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 6208 W: 1849 L: 1668 D: 2691

  ELO   | 7.77 +- 5.23 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 7600 W: 1790 L: 1620 D: 4190
  ```

- Created a "real" time managenement system, using the type of bestmove found
  (capture with very large SEE, checking move, only-move, etc.), the eval
  direction (the more it raises, the quicker the move is played), and the
  bestmove stability (stable bestmoves being played quicker). Increased
  max time usage to time / sqrt(movestogo) so that iterations have a high
  chance of being finished before sending the bestmove.
  ```
  ELO   | 30.96 +- 12.32 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 1744 W: 572 L: 417 D: 755

  ELO   | 29.02 +- 11.82 (95%)
  SPRT  | 40/10.0s Threads=1 Hash=16MB
  LLR   | 3.02 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 1872 W: 603 L: 447 D: 822
  ```

- Moved stand-pat evaluation after TT probing in Quiescence Search.
  ```
  ELO   | 35.68 +- 13.22 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 1456 W: 471 L: 322 D: 663

  ELO   | 28.46 +- 10.64 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 2.96 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 1664 W: 406 L: 270 D: 988
  ```

- Added a scaling function for endgames. (Note: this patch has been done via two
  different methods, the first one was using a material key and mapping endgames
  to specialized evals, the second only used the material quantity to evaluate
  the "drawishness" of the eval. The second one looked slightly worse, but
  allowed me to remove about 550+ lines of code with little to no effect. Here's
  the result for the initial version:
  ```
  ELO   | 3.94 +- 3.09 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 3.03 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 20192 W: 4330 L: 4101 D: 11761
  ```
  And here's the result for the massive simplification relative to the initial
  version:
  ```
  ELO   | -1.65 +- 4.12 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | -2.95 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 13472 W: 3295 L: 3359 D: 6818
  ```
  So the overall gain is likely about 2-3 Elo.)

- Increased LMR by one ply for non-PV nodes.
  ```
  ELO   | 5.09 +- 3.92 (95%)
  SPRT  | 10.0+0.1s Threads=1 Hash=16MB
  LLR   | 3.00 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 14944 W: 3819 L: 3600 D: 7525

  ELO   | 4.57 +- 3.48 (95%)
  SPRT  | 60.0+0.6s Threads=1 Hash=64MB
  LLR   | 3.01 (-2.94, 2.94) [0.00, 5.00]
  Games | N: 15354 W: 3190 L: 2988 D: 9176
  ```

### Changed (1 change)

- Added a new option structure to easily add UCI options for tuning sessions,
  with any framework like SPSA, chess-tuning-tools or such.

