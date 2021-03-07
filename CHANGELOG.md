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

