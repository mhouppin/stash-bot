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

#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movepick.h"
#include "timeman.h"
#include "tt.h"
#include "types.h"
#include "uci.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int Reductions[2][256];

double LmrNoisyBase = 4.15;
double LmrNoisyFactor = 10.81;
double LmrQuietBase = 10.69;
double LmrQuietFactor = 20.76;

long LmrBase = -415;
long LmrImproving = 538;

long LmpGoodBase = 63;
long LmpGoodFactor = 8;
long LmpBadBase = 13;
long LmpBadFactor = 4;

score_t DeltaBase = 8;
score_t DeltaDiv = 82;
score_t DeltaScale = 79;

score_t RazoringDelta = 135;

long RfpDepth = 8;
score_t RfpFactor = 85;
score_t RfpImproving = 73;

long NmpBase = 792;
long NmpFactor = 67;
score_t NmpEvalDiv = 109;
long NmpEvalMax = 5;
long NmpVerifDepth = 12;

score_t ProbCutDelta = 140;
long ProbCutDepth = 6;
long ProbCutEntryDepth = 4;
long ProbCutReduction = 3;

long IirDepth = 3;

long LmpDepth = 9;

long FpDepth = 7;
score_t FpBase = 186;
score_t FpFactor = 67;

long ChpDepth = 4;
long ChpBase = 842;
long ChpFactor = 5678;

long SeeDepth = 12;
score_t SeeQuiet = 49;
score_t SeeNoisy = 22;

long SingularDepth = 8;
long SingularEntryDepth = 3;
score_t SingularBetaFactor = 11;
long SingularShift = 2;
score_t SingularScoreDE = 17;
long SingularMaxDE = 9;

long LmrHistoryDiv = 12614;
long LmrHistoryMax = 3;

long QsFpMinPieces = 5;
score_t QsFpDelta = 110;

void init_search_tables(void)
{
    // Compute the LMR base values.
    for (int i = 1; i < 256; ++i)
    {
        Reductions[0][i] = (int)(log(i) * LmrNoisyFactor + LmrNoisyBase); // Noisy LMR formula
        Reductions[1][i] = (int)(log(i) * LmrQuietFactor + LmrQuietBase); // Quiet LMR formula
    }
}

int lmr_base_value(int depth, int movecount, bool improving, bool isQuiet)
{
    return (LmrBase + Reductions[isQuiet][depth] * Reductions[isQuiet][movecount] + !improving * LmrImproving)
           / 1024;
}

int lmp_threshold(int depth, bool improving)
{
    int result = improving ? LmpGoodBase + LmpGoodFactor * depth * depth : LmpBadBase + LmpBadFactor * depth * depth;

    return result / 16;
}

void init_searchstack(Searchstack *ss)
{
    memset(ss, 0, sizeof(Searchstack) * 256);

    for (int i = 0; i < 256; ++i) (ss + i)->plies = i - 4;
}

int get_conthist_score(const Board *board, const Searchstack *ss, move_t move)
{
    const piece_t movedPiece = piece_on(board, from_sq(move));
    const square_t to = to_sq(move);
    int history = 0;

    if ((ss - 1)->pieceHistory != NULL)
        history += get_pc_history_score(*(ss - 1)->pieceHistory, movedPiece, to);

    if ((ss - 2)->pieceHistory != NULL)
        history += get_pc_history_score(*(ss - 2)->pieceHistory, movedPiece, to);

    if ((ss - 4)->pieceHistory != NULL)
        history += get_pc_history_score(*(ss - 4)->pieceHistory, movedPiece, to);

    return history;
}

int get_history_score(const Board *board, const Worker *worker, const Searchstack *ss, move_t move)
{
    const piece_t movedPiece = piece_on(board, from_sq(move));

    return get_bf_history_score(worker->bfHistory, movedPiece, move)
           + get_conthist_score(board, ss, move);
}

uint64_t perft(Board *board, unsigned int depth)
{
    if (depth == 0) return 1;

    Movelist list;
    list_all(&list, board);

    // Bulk counting: the perft number at depth 1 equals the number of legal
    // moves. This is a large perft speedup from not having to do the
    // make/unmake move stuff.
    if (depth == 1) return movelist_size(&list);

    uint64_t sum = 0;
    Boardstack stack;

    for (ExtendedMove *extmove = list.moves; extmove < list.last; ++extmove)
    {
        do_move(board, extmove->move, &stack);
        sum += perft(board, depth - 1);
        undo_move(board, extmove->move);
    }

    return sum;
}

void update_pv(move_t *pv, move_t bestmove, move_t *subPv)
{
    size_t i;

    // Copy the sub-PV to the PV array, by adding the bestmove to the front.
    pv[0] = bestmove;
    for (i = 0; subPv[i] != NO_MOVE; ++i) pv[i + 1] = subPv[i];

    pv[i + 1] = NO_MOVE;
}

void main_worker_search(Worker *worker)
{
    Board *board = &worker->board;

    // Special case for perft searches.
    if (UciSearchParams.perft)
    {
        clock_t time = chess_clock();
        uint64_t nodes = perft(board, (unsigned int)UciSearchParams.perft);

        time = chess_clock() - time;

        uint64_t nps = nodes / (time + !time) * 1000;

        printf("info nodes %" FMT_INFO " nps %" FMT_INFO " time %" FMT_INFO "\n", (info_t)nodes,
            (info_t)nps, (info_t)time);

        return;
    }

    // Stop the search here if there exists no legal moves due to
    // checkmate/stalemate.
    if (worker->rootCount == 0)
    {
        printf("info depth 0 score %s 0\n", (board->stack->checkers) ? "mate" : "cp");
        fflush(stdout);
    }
    else
    {
        // The main thread initializes all the shared things for search here:
        // node counter, time manager, workers' board and threads, and TT reset.
        tt_clear();
        init_search_tables();
        wpool_new_search(&SearchWorkerPool);

        if (UciSearchParams.depth == 0) UciSearchParams.depth = MAX_PLIES;

        if (UciSearchParams.nodes == 0) --UciSearchParams.nodes;

        wpool_start_workers(&SearchWorkerPool);
        worker_search(worker);
    }

    // UCI protocol specifies that we shouldn't send the bestmove command
    // before the GUI sends us the "stop" in infinite mode
    // or "ponderhit" in ponder mode.
    while (!wpool_is_stopped(&SearchWorkerPool)
           && (wpool_is_pondering(&SearchWorkerPool) || UciSearchParams.infinite));

    wpool_stop(&SearchWorkerPool);

    if (worker->rootCount == 0)
    {
        puts("bestmove 0000");
        fflush(stdout);
        free(worker->rootMoves);
        free_boardstack(worker->stack);
        return;
    }

    // Wait for all threads to stop searching.
    wpool_wait_search_end(&SearchWorkerPool);

    printf("bestmove %s", move_to_str(worker->rootMoves->move, board->chess960));

    move_t ponderMove = worker->rootMoves->pv[1];

    // If we finished searching with a fail-high, try to see if we can get a ponder
    // move in TT.
    if (ponderMove == NO_MOVE)
    {
        Boardstack stack;
        TT_Entry *entry;
        bool found;

        do_move(board, worker->rootMoves->move, &stack);
        entry = tt_probe(board->stack->boardKey, &found);
        undo_move(board, worker->rootMoves->move);

        if (found)
        {
            ponderMove = entry->bestmove;

            // Take care of data races !
            if (!move_is_pseudo_legal(board, ponderMove) || !move_is_legal(board, ponderMove))
                ponderMove = NO_MOVE;
        }
    }

    if (ponderMove != NO_MOVE) printf(" ponder %s", move_to_str(ponderMove, board->chess960));

    putchar('\n');
    fflush(stdout);

    free(worker->rootMoves);
    free_boardstack(worker->stack);
}

// Does a whole search iteration on all root moves, MultiPV and aspiration re-searches included.
void do_search_iteration(Worker *worker, int depth, int multiPv, Searchstack *sstack)
{
    for (worker->pvLine = 0; worker->pvLine < multiPv; ++worker->pvLine)
    {
        // Reset the seldepth value after each depth increment, and for each
        // PV line.
        worker->seldepth = 0;

        score_t alpha, beta, delta;
        score_t pvScore = worker->rootMoves[worker->pvLine].prevScore;
        int bound = NO_BOUND;

        // Don't set aspiration window bounds for low depths, as the scores are
        // very volatile.
        if (depth <= 8)
        {
            delta = 0;
            alpha = -INF_SCORE;
            beta = INF_SCORE;
        }
        else
        {
            delta = DeltaBase + abs(pvScore) / DeltaDiv;
            alpha = imax(-INF_SCORE, pvScore - delta);
            beta = imin(INF_SCORE, pvScore + delta);
        }

        do {
            search(true, &worker->board, depth, alpha, beta, sstack + 4, false);
            sort_root_moves(
                worker->rootMoves + worker->pvLine, worker->rootMoves + worker->rootCount);
            pvScore = worker->rootMoves[worker->pvLine].score;

            // Note: we set the bound to be EXACT_BOUND when the search aborts, even if the last
            // search finished on a fail low/high (this also allows us to exit the search loop
            // without checking for a search stop again).
            bound = wpool_is_stopped(&SearchWorkerPool) ? EXACT_BOUND
                    : (pvScore >= beta)                 ? LOWER_BOUND
                    : (pvScore <= alpha)                ? UPPER_BOUND
                                                        : EXACT_BOUND;

            if (bound == EXACT_BOUND)
                sort_root_moves(worker->rootMoves, worker->rootMoves + multiPv);

            if (!worker->idx)
            {
                clock_t time = chess_clock() - SearchTimeman.start;
                bool lateInfo = time > 3000;
                bool singlePv = multiPv == 1;
                bool iterCompleted = worker->pvLine == multiPv - 1;

                // Don't update Multi-PV lines if they are not all analysed at current depth
                // and not enough time has passed to avoid flooding the standard output.
                if ((lateInfo && singlePv) || (bound == EXACT_BOUND && (lateInfo || iterCompleted)))
                {
                    for (int i = 0; i < multiPv; ++i)
                        print_pv(&worker->board, worker->rootMoves + i, i + 1, worker->rootDepth,
                            time, bound);

                    fflush(stdout);
                }
            }

            // Update aspiration window bounds in case of fail low/high.
            if (bound == UPPER_BOUND)
            {
                depth = worker->rootDepth;
                beta = (alpha + beta) / 2;
                alpha = imax(-INF_SCORE, (int)pvScore - delta);
            }
            else if (bound == LOWER_BOUND)
            {
                depth -= (depth - 1 > (worker->rootDepth - 1) / 2);
                beta = imin(INF_SCORE, (int)pvScore + delta);
            }

            delta += delta * DeltaScale / 256;

        } while (bound != EXACT_BOUND);

        // Don't search other PV lines if the search aborted.
        if (wpool_is_stopped(&SearchWorkerPool)) break;
    }
}

void worker_search(Worker *worker)
{
    // Clamp MultiPV to the maximal number of lines available.
    int multiPv = imin(UciOptionFields.multiPv, worker->rootCount);
    Searchstack sstack[256];

    init_searchstack(sstack);

    for (worker->rootDepth = 1; worker->rootDepth <= UciSearchParams.depth; ++worker->rootDepth)
    {
        do_search_iteration(worker, worker->rootDepth, multiPv, sstack);

        // Reset root moves' score for the next search.
        for (RootMove *i = worker->rootMoves; i < worker->rootMoves + worker->rootCount; ++i)
        {
            i->prevScore = i->score;
            i->score = -INF_SCORE;
        }

        if (wpool_is_stopped(&SearchWorkerPool)) break;

        // If we went over optimal time usage, we just finished our iteration,
        // so we can safely return our bestmove.
        if (!worker->idx)
        {
            timeman_update(&SearchTimeman, &worker->board, worker->rootMoves->move,
                worker->rootMoves->prevScore);
            if (timeman_can_stop_search(&SearchTimeman, chess_clock())) break;
        }

        // If we're searching for mate and have found a mate equal or better than the given one,
        // stop the search.
        if (UciSearchParams.mate
            && worker->rootMoves->prevScore >= mate_in(UciSearchParams.mate * 2))
            break;

        // During fixed depth or infinite searches, allow the non-main workers to keep searching
        // as long as the main worker hasn't finished.
        if (worker->idx && worker->rootDepth == UciSearchParams.depth) --worker->rootDepth;
    }

    if (worker->idx)
    {
        free(worker->rootMoves);
        free_boardstack(worker->stack);
    }
}

score_t search(bool pvNode, Board *board, int depth, score_t alpha, score_t beta, Searchstack *ss,
    bool cutNode)
{
    bool rootNode = (ss->plies == 0);
    Worker *worker = get_worker(board);

    // Perform an early check for repetition detections.
    if (!rootNode && board->stack->rule50 >= 3 && alpha < 0 && game_has_cycle(board, ss->plies))
    {
        alpha = draw_score(worker);
        if (alpha >= beta) return alpha;
    }

    // Drop into qsearch if the depth isn't strictly positive.
    if (depth <= 0) return qsearch(pvNode, board, alpha, beta, ss);

    Movepicker mp;
    move_t pv[256];
    score_t bestScore = -INF_SCORE;

    // Verify the time usage if we're the main thread.
    if (!worker->idx) check_time();

    // Update the seldepth value if needed.
    if (pvNode && worker->seldepth < ss->plies + 1) worker->seldepth = ss->plies + 1;

    // Stop the search if the game is drawn or the timeman/UCI thread asks for a stop.
    if (!rootNode && (wpool_is_stopped(&SearchWorkerPool) || game_is_drawn(board, ss->plies)))
        return draw_score(worker);

    // Stop the search after MAX_PLIES recursive search calls.
    if (ss->plies >= MAX_PLIES)
        return !board->stack->checkers ? evaluate(board) : draw_score(worker);

    if (!rootNode)
    {
        // Mate distance pruning.
        alpha = imax(alpha, mated_in(ss->plies));
        beta = imin(beta, mate_in(ss->plies + 1));

        if (alpha >= beta) return alpha;
    }

    bool inCheck = !!board->stack->checkers;
    bool improving;

    // Check for interesting TT values.
    int ttDepth = 0;
    int ttBound = NO_BOUND;
    score_t ttScore = NO_SCORE;
    move_t ttMove = NO_MOVE;
    bool ttNoisy = false;
    bool found;
    hashkey_t key = board->stack->boardKey ^ ((hashkey_t)ss->excludedMove << 16);
    TT_Entry *entry = tt_probe(key, &found);
    score_t rawEval;
    score_t eval;

    if (found)
    {
        ttScore = score_from_tt(entry->score, ss->plies);
        ttBound = entry->genbound & 3;
        ttDepth = entry->depth;
        ttMove = entry->bestmove;

        // Check if we can directly return a score for non-PV nodes.
        if (ttDepth >= depth && !pvNode)
            if (((ttBound & LOWER_BOUND) && ttScore >= beta)
                || ((ttBound & UPPER_BOUND) && ttScore <= alpha))
            {
                if ((ttBound & LOWER_BOUND) && !is_capture_or_promotion(board, ttMove))
                    update_quiet_history(board, depth, ttMove, NULL, 0, ss);

                return ttScore;
            }

        ttNoisy = ttMove && is_capture_or_promotion(board, ttMove);
    }

    (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;
    ss->doubleExtensions = (ss - 1)->doubleExtensions;

    // Don't perform early pruning or compute the eval while in check.
    if (inCheck)
    {
        eval = ss->staticEval = rawEval = NO_SCORE;
        improving = false;
        goto main_loop;
    }
    // Use the TT stored information for getting an eval.
    else if (found)
    {
        rawEval = entry->eval;
        eval = ss->staticEval =
            rawEval + get_correction(worker->corrHistory, board->sideToMove, get_pawn_key(board));

        // Try to use the TT score as a better evaluation of the position.
        if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND)) eval = ttScore;
    }
    // Call the evaluation function otherwise.
    else
    {
        rawEval = evaluate(board);
        eval = ss->staticEval =
            rawEval + get_correction(worker->corrHistory, board->sideToMove, get_pawn_key(board));

        // Save the eval in TT so that other workers won't have to recompute it.
        tt_save(entry, key, NO_SCORE, rawEval, 0, NO_BOUND, NO_MOVE);
    }

    if (rootNode && worker->pvLine) ttMove = worker->rootMoves[worker->pvLine].move;

    // Razoring. If our static eval isn't good, and depth is low, it is likely
    // that only a capture will save us at this stage. Drop into qsearch.
    if (!pvNode && depth == 1 && ss->staticEval + RazoringDelta <= alpha)
        return qsearch(false, board, alpha, beta, ss);

    improving = ss->plies >= 2 && ss->staticEval > (ss - 2)->staticEval;

    // Futility Pruning. If our eval is quite good and depth is low, we just
    // assume that we won't fall far behind in the next plies, and we return the
    // eval.
    if (!pvNode && depth <= RfpDepth && eval - imax(RfpFactor * depth - RfpImproving * improving, 0) >= beta && eval < VICTORY)
        return eval;

    // Null Move Pruning. If our eval currently beats beta, and we still have
    // non-Pawn material on the board, we try to see what happens if we skip our
    // turn. If the resulting reduced search still beats beta, we assume our
    // position is so good that we cannot get under beta at this point.
    if (!pvNode && depth >= 3 && ss->plies >= worker->verifPlies && !ss->excludedMove
        && eval >= beta && eval >= ss->staticEval && board->stack->material[board->sideToMove])
    {
        Boardstack stack;

        // Compute the depth reduction based on depth and eval difference with beta.
        int r = (NmpBase + NmpFactor * depth) / 256 + imin((eval - beta) / NmpEvalDiv, NmpEvalMax);

        ss->currentMove = NULL_MOVE;
        ss->pieceHistory = NULL;

        do_null_move(board, &stack);
        atomic_fetch_add_explicit(&get_worker(board)->nodes, 1, memory_order_relaxed);

        // Perform the reduced search.
        score_t score = -search(false, board, depth - r, -beta, -beta + 1, ss + 1, !cutNode);
        undo_null_move(board);

        if (score >= beta)
        {
            // Do not trust mate claims, as we don't want to return false mate
            // scores due to zugzwang. Adjust the score acordingly.
            if (score > MATE_FOUND) score = beta;

            // Do not trust win claims for the same reason as above, and do not
            // return early for high-depth searches.
            if (worker->verifPlies || (depth <= NmpVerifDepth && abs(beta) < VICTORY)) return score;

            // Zugzwang checking. For high depth nodes, we perform a second
            // reduced search at the same depth, but this time with NMP disabled
            // for a few plies. If this search still beats beta, we assume to
            // not be in a zugzwang situation, and return the previous reduced
            // search score.
            worker->verifPlies = ss->plies + (depth - r) * 3 / 4;
            score_t zzscore = search(false, board, depth - r, beta - 1, beta, ss, false);
            worker->verifPlies = 0;

            if (zzscore >= beta) return score;
        }
    }

    // Probcut. If we have a good enough capture (or promotion) and a reduced
    // search returns a value much above beta, we can (almost) safely prune the
    // previous move.
    const score_t probCutBeta = beta + ProbCutDelta;

    if (!rootNode && depth >= ProbCutDepth && abs(beta) < VICTORY
        && !(found && ttDepth >= depth - ProbCutEntryDepth && ttScore < probCutBeta))
    {
        const score_t probCutSEE = probCutBeta - ss->staticEval;
        move_t currmove;

        movepicker_init(&mp, true, board, worker,
            ttMove && see_greater_than(board, ttMove, probCutSEE) ? ttMove : NO_MOVE, ss);

        while ((currmove = movepicker_next_move(&mp, false, probCutSEE)) != NO_MOVE)
        {
            if (mp.stage == PICK_BAD_INSTABLE) break;

            if (!move_is_legal(board, currmove) || currmove == ss->excludedMove) continue;

            ss->currentMove = currmove;
            ss->pieceHistory =
                &worker->ctHistory[piece_on(board, from_sq(currmove))][to_sq(currmove)];

            Boardstack stack;
            bool givesCheck = move_gives_check(board, currmove);
            do_move_gc(board, currmove, &stack, givesCheck);
            atomic_fetch_add_explicit(&get_worker(board)->nodes, 1, memory_order_relaxed);

            score_t probCutScore = -qsearch(false, board, -probCutBeta, -probCutBeta + 1, ss + 1);

            if (probCutScore >= probCutBeta)
                probCutScore = -search(
                    false, board, depth - ProbCutReduction - 1, -probCutBeta, -probCutBeta + 1, ss + 1, !cutNode);

            undo_move(board, currmove);

            if (probCutScore >= probCutBeta)
            {
                tt_save(entry, key, score_to_tt(probCutScore, ss->plies), rawEval, depth - ProbCutReduction,
                    LOWER_BOUND, currmove);
                return probCutScore;
            }
        }
    }

    // Reduce depth if the node is absent from TT.
    if (!rootNode && !found && depth >= IirDepth) --depth;

main_loop:
    movepicker_init(&mp, false, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;
    move_t quiets[64];
    int qcount = 0;
    move_t captures[64];
    int ccount = 0;
    bool skipQuiets = false;

    while ((currmove = movepicker_next_move(&mp, skipQuiets, 0)) != NO_MOVE)
    {
        if (rootNode)
        {
            // Exclude already searched PV lines for root nodes.
            if (find_root_move(worker->rootMoves + worker->pvLine,
                    worker->rootMoves + worker->rootCount, currmove)
                == NULL)
                continue;
        }
        else
        {
            if (!move_is_legal(board, currmove) || currmove == ss->excludedMove) continue;
        }

        moveCount++;

        bool isQuiet = !is_capture_or_promotion(board, currmove);

        if (!rootNode && bestScore > -MATE_FOUND)
        {
            // Late Move Pruning. For low-depth nodes, stop searching quiets
            // after a certain movecount has been reached.
            if (depth <= LmpDepth && moveCount >= lmp_threshold(depth, improving)) skipQuiets = true;

            // Futility Pruning. For low-depth nodes, stop searching quiets if
            // the eval suggests that only captures will save the day.
            if (depth <= FpDepth && !inCheck && isQuiet && eval + FpBase + FpFactor * depth <= alpha)
                skipQuiets = true;

            // Continuation History Pruning. For low-depth nodes, prune quiet moves if
            // they seem to be bad continuations to the previous moves.
            if (depth <= ChpDepth && get_conthist_score(board, ss, currmove) < ChpBase - ChpFactor * (depth - 1))
                continue;

            // SEE Pruning. For low-depth nodes, don't search moves which seem
            // to lose too much material to be interesting.
            if (depth <= 12
                && !see_greater_than(
                    board, currmove, (isQuiet ? -49 * depth : -22 * depth * depth)))
                continue;
        }

        // Report currmove info if enough time has passed.
        if (rootNode && !worker->idx && chess_clock() - SearchTimeman.start > 3000)
        {
            printf("info depth %d currmove %s currmovenumber %d\n", depth,
                move_to_str(currmove, board->chess960), moveCount + worker->pvLine);
            fflush(stdout);
        }

        Boardstack stack;
        score_t score = -NO_SCORE;
        int extension = 0;
        int newDepth = depth - 1;
        bool givesCheck = move_gives_check(board, currmove);
        int histScore = isQuiet ? get_history_score(board, worker, ss, currmove) : 0;

        if (!rootNode && ss->plies < 2 * worker->rootDepth
            && 2 * ss->doubleExtensions < worker->rootDepth)
        {
            // Singular Extensions. For high-depth nodes, if the TT entry
            // suggests that the TT move is really good, we check if there are
            // other moves which maintain the score close to the TT score. If
            // that's not the case, we consider the TT move to be singular, and
            // we extend non-LMR searches by one or two lies, depending on the
            // margin that the singular search failed low.
            if (depth >= SingularDepth && currmove == ttMove && !ss->excludedMove && (ttBound & LOWER_BOUND)
                && abs(ttScore) < VICTORY && ttDepth >= depth - SingularEntryDepth)
            {
                score_t singularBeta = ttScore - SingularBetaFactor * depth / 16;
                int singularDepth = (depth + SingularShift) / 2;

                // Exclude the TT move from the singular search.
                ss->excludedMove = ttMove;
                score_t singularScore = search(
                    false, board, singularDepth, singularBeta - 1, singularBeta, ss, cutNode);
                ss->excludedMove = NO_MOVE;

                // Our singular search failed to produce a cutoff, extend the TT
                // move.
                if (singularScore < singularBeta)
                {
                    if (!pvNode && singularBeta - singularScore > SingularScoreDE && ss->doubleExtensions <= SingularMaxDE)
                    {
                        extension = 2;
                        ss->doubleExtensions++;
                    }
                    else
                        extension = 1;
                }

                // Multicut Pruning. If our singular search produced a cutoff,
                // and the search bounds were equal or superior to our normal
                // search, assume that there are multiple moves that beat beta
                // in the current node, and return a search score early.
                else if (singularBeta >= beta)
                    return singularBeta;

                // Negative Extensions. If our singular search produced a cutoff,
                // with singularBeta was too low to beat beta, but the TT entry
                // having a search score above beta, we assume that searching the
                // TT move at full depth is futile as we should get a fail-high
                // deeper on this branch, and reduce its search depth.
                else if (ttScore >= beta)
                    extension = -1;
            }
            // Check Extensions. Extend non-LMR searches by one ply for moves
            // that give check.
            else if (givesCheck)
                extension = 1;
        }

        piece_t movedPiece = piece_on(board, from_sq(currmove));

        // Save the piece history for the current move so that sub-nodes can use
        // it for ordering moves.
        ss->currentMove = currmove;
        ss->pieceHistory = &worker->ctHistory[movedPiece][to_sq(currmove)];

        do_move_gc(board, currmove, &stack, givesCheck);
        atomic_fetch_add_explicit(&get_worker(board)->nodes, 1, memory_order_relaxed);

        // Late Move Reductions. For nodes not too close to qsearch (since
        // we can't reduce their search depth), we start reducing moves after
        // a certain movecount has been reached, as we consider them less likely
        // to produce cutoffs in standard searches.
        if (depth >= 3 && moveCount > 1 + 3 * pvNode)
        {
            // Set the base depth reduction value based on depth and
            // movecount.
            int r = lmr_base_value(depth, moveCount, improving, isQuiet);

            // Increase the reduction for non-PV nodes.
            r += !pvNode;

            // Increase the reduction for cutNodes.
            r += cutNode;

            // Increase the reduction if the TT move is non-quiet.
            r += ttNoisy;

            // Decrease the reduction if the move is a killer or countermove.
            r -= (currmove == mp.killer1 || currmove == mp.killer2 || currmove == mp.counter);

            // Decrease the reduction if the move escapes a capture.
            r -= isQuiet && !see_greater_than(board, reverse_move(currmove), 0);

            // Increase/decrease the reduction based on the move's history.
            r -= iclamp(histScore / LmrHistoryDiv, -LmrHistoryMax, LmrHistoryMax);

            // Clamp the reduction so that we don't extend the move or drop
            // immediately into qsearch.
            r = iclamp(r, 0, newDepth - 1);

            score = -search(false, board, newDepth - r, -alpha - 1, -alpha, ss + 1, true);

            // Perform another search at full depth if LMR failed high.
            if (r != 0 && score > alpha)
            {
                score = -search(
                    false, board, newDepth + extension, -alpha - 1, -alpha, ss + 1, !cutNode);

                update_cont_histories(ss, depth, movedPiece, to_sq(currmove), score > alpha);
            }
        }
        // If LMR is not possible, do a search with no reductions.
        else if (!pvNode || moveCount != 1)
            score =
                -search(false, board, newDepth + extension, -alpha - 1, -alpha, ss + 1, !cutNode);

        // In PV nodes, perform an additional full-window search for the first
        // move, or when all our previous searches returned fail-highs.
        if (pvNode && (moveCount == 1 || score > alpha))
        {
            (ss + 1)->pv = pv;
            pv[0] = NO_MOVE;
            score = -search(true, board, newDepth + extension, -beta, -alpha, ss + 1, false);
        }

        undo_move(board, currmove);

        // Check for search abortion here.
        if (wpool_is_stopped(&SearchWorkerPool)) return 0;

        if (rootNode)
        {
            RootMove *cur = find_root_move(worker->rootMoves + worker->pvLine,
                worker->rootMoves + worker->rootCount, currmove);

            // Update the PV in root nodes for the first move, and for all moves
            // beating alpha.
            if (moveCount == 1 || score > alpha)
            {
                cur->score = score;
                cur->seldepth = worker->seldepth;
                cur->pv[0] = currmove;

                update_pv(cur->pv, currmove, (ss + 1)->pv);
            }
            else
                cur->score = -INF_SCORE;
        }

        // Check if our score improves the current best score.
        if (bestScore < score)
        {
            bestScore = score;

            // Check if our score beats alpha.
            if (alpha < bestScore)
            {
                bestmove = currmove;
                alpha = bestScore;

                // Update the PV for PV nodes.
                if (pvNode && !rootNode) update_pv(ss->pv, currmove, (ss + 1)->pv);

                // Check if our move generates a cutoff, in which case we can
                // stop searching other moves at this node.
                if (alpha >= beta)
                {
                    // Update move histories.
                    if (isQuiet) update_quiet_history(board, depth, bestmove, quiets, qcount, ss);
                    if (moveCount != 1)
                        update_capture_history(board, depth, bestmove, captures, ccount, ss);
                    break;
                }
            }
        }

        // Keep track of all moves that failed to generate a cutoff.
        if (qcount < 64 && isQuiet)
            quiets[qcount++] = currmove;
        else if (ccount < 64 && !isQuiet)
            captures[ccount++] = currmove;
    }

    // Are we in checkmate/stalemate ? Take care of not returning a wrong draw
    // or mate score in singular searches.
    if (moveCount == 0)
        bestScore = (ss->excludedMove) ? alpha : (board->stack->checkers) ? mated_in(ss->plies) : 0;

    int bound = (bestScore >= beta)    ? LOWER_BOUND
                : (pvNode && bestmove) ? EXACT_BOUND
                                       : UPPER_BOUND;

    // If we're not in check, and we don't have a tactical best-move,
    // and the static eval needs moving in a direction, then update corrhist.
    if (!(board->stack->checkers || (bestmove && is_capture_or_promotion(board, bestmove))
            || (bound == LOWER_BOUND && bestScore <= ss->staticEval)
            || (bound == UPPER_BOUND && bestScore >= ss->staticEval)))
    {
        add_correction_history(worker->corrHistory, board->sideToMove, get_pawn_key(board), depth,
            bestScore - ss->staticEval);
    }

    // Only save TT for the first MultiPV move in root nodes.
    if (!rootNode || worker->pvLine == 0)
    {
        tt_save(entry, key, score_to_tt(bestScore, ss->plies), rawEval, depth, bound, bestmove);
    }

    return bestScore;
}

score_t qsearch(bool pvNode, Board *board, score_t alpha, score_t beta, Searchstack *ss)
{
    Worker *worker = get_worker(board);
    const score_t oldAlpha = alpha;
    Movepicker mp;
    move_t pv[256];

    // Verify the time usage if we're the main thread.
    if (!worker->idx) check_time();

    // Update the seldepth value if needed.
    if (pvNode && worker->seldepth < ss->plies + 1) worker->seldepth = ss->plies + 1;

    // Stop the search if the game is drawn or the timeman/UCI thread asks for a stop.
    if (wpool_is_stopped(&SearchWorkerPool) || game_is_drawn(board, ss->plies))
        return draw_score(worker);

    // Stop the search after MAX_PLIES recursive search calls.
    if (ss->plies >= MAX_PLIES)
        return !board->stack->checkers ? evaluate(board) : draw_score(worker);

    // Mate distance pruning.
    alpha = imax(alpha, mated_in(ss->plies));
    beta = imin(beta, mate_in(ss->plies + 1));

    if (alpha >= beta) return alpha;

    // Check for interesting TT values.
    score_t ttScore = NO_SCORE;
    int ttBound = NO_BOUND;
    bool found;
    TT_Entry *entry = tt_probe(board->stack->boardKey, &found);

    if (found)
    {
        ttBound = entry->genbound & 3;
        ttScore = score_from_tt(entry->score, ss->plies);

        // Check if we can directly return a score for non-PV nodes.
        if (!pvNode
            && (((ttBound & LOWER_BOUND) && ttScore >= beta)
                || ((ttBound & UPPER_BOUND) && ttScore <= alpha)))
            return ttScore;
    }

    bool inCheck = !!board->stack->checkers;
    score_t rawEval;
    score_t eval;
    score_t bestScore;

    // Don't compute the eval while in check.
    if (inCheck)
    {
        rawEval = NO_SCORE;
        eval = NO_SCORE;
        bestScore = -INF_SCORE;
    }
    else
    {
        // Use the TT stored information for getting an eval.
        if (found)
        {
            rawEval = entry->eval;
            eval = bestScore =
                rawEval
                + get_correction(worker->corrHistory, board->sideToMove, get_pawn_key(board));

            // Try to use the TT score as a better evaluation of the position.
            if (ttBound & (ttScore > eval ? LOWER_BOUND : UPPER_BOUND)) bestScore = ttScore;
        }
        // Call the evaluation function otherwise.
        else
        {
            rawEval = evaluate(board);
            eval = bestScore =
                rawEval
                + get_correction(worker->corrHistory, board->sideToMove, get_pawn_key(board));
        }

        // Stand Pat. If not playing a capture is better because of better quiet
        // moves, allow for a simple eval return.
        alpha = imax(alpha, bestScore);
        if (alpha >= beta)
        {
            // Save the eval in TT so that other workers won't have to recompute it.
            if (!found)
                tt_save(entry, board->stack->boardKey, score_to_tt(bestScore, ss->plies), rawEval,
                    0, LOWER_BOUND, NO_MOVE);
            return alpha;
        }
    }

    move_t ttMove = entry->bestmove;

    movepicker_init(&mp, true, board, worker, ttMove, ss);

    move_t currmove;
    move_t bestmove = NO_MOVE;
    int moveCount = 0;

    if (pvNode) (ss + 1)->pv = pv;

    // Check if Futility Pruning is possible in the moves loop.
    const bool canFutilityPrune = (!inCheck && popcount(occupancy_bb(board)) >= QsFpMinPieces);
    const score_t futilityBase = bestScore + QsFpDelta;

    while ((currmove = movepicker_next_move(&mp, false, 0)) != NO_MOVE)
    {
        // Only analyse good capture moves.
        if (bestScore > -MATE_FOUND && mp.stage == PICK_BAD_INSTABLE) break;

        if (!move_is_legal(board, currmove)) continue;

        moveCount++;

        bool givesCheck = move_gives_check(board, currmove);

        // Futility Pruning. If we already have non-mating score and our move
        // doesn't give check, test if playing it has a chance to make the score
        // go over alpha.
        if (bestScore > -MATE_FOUND && canFutilityPrune && !givesCheck
            && move_type(currmove) == NORMAL_MOVE)
        {
            score_t delta = futilityBase + PieceScores[ENDGAME][piece_on(board, to_sq(currmove))];

            // Check if the move is unlikely to improve alpha.
            if (delta < alpha) continue;

            // If static eval is far below alpha, only search moves that win material.
            if (futilityBase < alpha && !see_greater_than(board, currmove, 1)) continue;
        }

        // Save the piece history for the current move so that sub-nodes can use
        // it for ordering moves.
        ss->currentMove = currmove;
        ss->pieceHistory = &worker->ctHistory[piece_on(board, from_sq(currmove))][to_sq(currmove)];

        Boardstack stack;

        if (pvNode) pv[0] = NO_MOVE;

        do_move_gc(board, currmove, &stack, givesCheck);
        atomic_fetch_add_explicit(&get_worker(board)->nodes, 1, memory_order_relaxed);

        score_t score = -qsearch(pvNode, board, -beta, -alpha, ss + 1);
        undo_move(board, currmove);

        // Check for search abortion here.
        if (wpool_is_stopped(&SearchWorkerPool)) return 0;

        // Check if our score improves the current best score.
        if (bestScore < score)
        {
            bestScore = score;

            // Check if our score beats alpha.
            if (alpha < bestScore)
            {
                alpha = bestScore;
                bestmove = currmove;

                // Update the PV for PV nodes.
                if (pvNode) update_pv(ss->pv, bestmove, (ss + 1)->pv);

                // Check if our move generates a cutoff, in which case we can
                // stop searching other moves at this node.
                if (alpha >= beta) break;
            }
        }
    }

    // Are we in checkmate ?
    if (moveCount == 0 && inCheck) bestScore = mated_in(ss->plies);

    int bound = (bestScore >= beta)       ? LOWER_BOUND
                : (bestScore <= oldAlpha) ? UPPER_BOUND
                                          : EXACT_BOUND;

    tt_save(entry, board->stack->boardKey, score_to_tt(bestScore, ss->plies), rawEval, 0, bound,
        bestmove);

    return bestScore;
}
