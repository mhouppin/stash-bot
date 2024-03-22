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

#include "uci.h"
#include "evaluate.h"
#include "movelist.h"
#include "option.h"
#include "timeman.h"
#include "tt.h"
#include "types.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UCI_VERSION "v35.19"

// clang-format off

static const CommandMap UciCommands[] =
{
    {"bench", &uci_bench},
    {"d", &uci_d},
    {"debug", &uci_debug},
    {"go", &uci_go},
    {"isready", &uci_isready},
    {"ponderhit", &uci_ponderhit},
    {"position", &uci_position},
    {"quit", &uci_quit},
    {"setoption", &uci_setoption},
    {"stop", &uci_stop},
    {"uci", &uci_uci},
    {"ucinewgame", &uci_ucinewgame},
    {NULL, NULL}
};

// clang-format on

static int uci_perr(const char *str) { return write(STDERR_FILENO, str, strlen(str)); }

__attribute__((noreturn)) static void uci_allocation_failure(const char *str)
{
    // Since we're OOM, all printf() like calls will likely fail.
    uci_perr("Unable to allocate ");
    uci_perr(str);
    uci_perr(": out of memory\n");
    exit(ENOMEM);
}

// Finds the next token in the given string, writes a nullbyte to its end,
// and returns it (after incrementing the string pointer).
// This is mainly used as a replacement to strtok_r(), which isn't available
// on MinGW.
char *get_next_token(char **str)
{
    while (isspace((unsigned char)**str) && **str != '\0') ++(*str);

    if (**str == '\0') return NULL;

    char *retval = *str;

    while (!isspace((unsigned char)**str) && **str != '\0') ++(*str);

    if (**str == '\0') return retval;

    **str = '\0';
    ++(*str);
    return retval;
}

const char *move_to_str(move_t move, bool isChess960)
{
    static char buf[6];

    // Handle side-cases early.
    if (move == NO_MOVE) return "none";

    if (move == NULL_MOVE) return "0000";

    square_t from = from_sq(move), to = to_sq(move);

    // We encode castling as "King takes Rook", so we have to fix the arrival
    // square for standard chess.
    if (move_type(move) == CASTLING && !isChess960)
        to = create_sq(to > from ? FILE_G : FILE_C, sq_rank(from));

    buf[0] = sq_file(from) + 'a';
    buf[1] = sq_rank(from) + '1';
    buf[2] = sq_file(to) + 'a';
    buf[3] = sq_rank(to) + '1';

    if (move_type(move) == PROMOTION)
    {
        buf[4] = " pnbrqk"[promotion_type(move)];
        buf[5] = '\0';
    }
    else
        buf[4] = '\0';

    return buf;
}

move_t str_to_move(const Board *board, const char *str)
{
    Movelist movelist;

    list_all(&movelist, board);

    // Try to find a legal move whose move string matches the given one.
    for (const ExtendedMove *m = movelist_begin(&movelist); m < movelist_end(&movelist); ++m)
    {
        const char *s = move_to_str(m->move, board->chess960);

        if (!strcmp(str, s)) return m->move;
    }

    return NO_MOVE;
}

int winrate_model(score_t score, int ply)
{
    // clang-format off
    static const double as[4] = {  -4.19608390,   27.38380265,   32.62081910,   98.27443919};
    static const double bs[4] = {   0.36809906,   -1.71950139,    9.71771959,   61.99845003};
    // clang-format on

    double p = imin(ply, 240) / 64.0;
    double a = ((as[0] * p + as[1]) * p + as[2]) * p + as[3];
    double b = ((bs[0] * p + bs[1]) * p + bs[2]) * p + bs[3];

    // Clamp the score value to avoid issues while computing the exponent.
    double v = fmin(fmax(-4000.0, (double)score), 4000.0);

    // Return the winrate in per mille units.
    return (int)(0.5 + 1000.0 / (1.0 + exp((a - v) / b)));
}

const char *score_to_wdl(score_t score, int ply)
{
    static char buf[17];

    if (!UciOptionFields.showWDL)
        buf[0] = '\0';

    else
    {
        int wdlWin = winrate_model(score, ply);
        int wdlLose = winrate_model(-score, ply);
        int wdlDraw = 1000 - wdlWin - wdlLose;

        sprintf(buf, " wdl %d %d %d", wdlWin, wdlDraw, wdlLose);
    }

    return buf;
}

const char *score_to_str(score_t score)
{
    static const score_t NormalizeScore = 154;
    static char buf[12];

    if (abs(score) >= MATE_FOUND)
        sprintf(buf, "mate %d", (score > 0 ? MATE - score + 1 : -MATE - score) / 2);

    else
    {
        if (UciOptionFields.normalizeScore) score = (int32_t)score * 100 / NormalizeScore;

        sprintf(buf, "cp %d", score);
    }

    return buf;
}

static uint64_t get_nps(uint64_t nodes, clock_t time)
{
    // Avoid division by zero
    time = timemax(time, 1);

    uint64_t nodesPerMillisecond = nodes / time;
    nodes -= nodesPerMillisecond * time;
    return (nodesPerMillisecond * 1000 + (nodes * 1000 / time));
}

void print_pv(
    const Board *board, RootMove *rootMove, int multiPv, int depth, clock_t time, int bound)
{
    static const char *BoundStr[] = {"", " upperbound", " lowerbound", ""};

    uint64_t nodes = wpool_get_total_nodes(&SearchWorkerPool);
    uint64_t nps = get_nps(nodes, time);
    bool searchedMove = (rootMove->score != -INF_SCORE);
    score_t rootScore = (searchedMove) ? rootMove->score : rootMove->prevScore;

    // At most 256 moves stored in (each taking 5 bytes) + 16 more bytes for
    // potential promotions + 1 byte for the final nullbyte.
    char pvBuffer[256 * 5 + 16 + 1];

    pvBuffer[0] = '\0';

    // Fill the PV buffer.
    for (size_t i = 0; rootMove->pv[i]; ++i)
    {
        strcat(pvBuffer, " ");
        strcat(pvBuffer, move_to_str(rootMove->pv[i], board->chess960));
    }

    // clang-format off
    printf("info"
        " depth %d"
        " seldepth %d"
        " multipv %d"
        " score %s%s%s"
        " nodes %" FMT_INFO
        " nps %" FMT_INFO
        " hashfull %d"
        " time %" FMT_INFO
        " pv%s\n",
        imax(depth - !searchedMove, 1),
        rootMove->seldepth,
        multiPv,
        score_to_str(rootScore), BoundStr[bound], score_to_wdl(rootScore, board->ply),
        (info_t)nodes,
        (info_t)nps,
        tt_hashfull(),
        (info_t)time,
        pvBuffer);
    // clang-format on
    fflush(stdout);
}

int debug_printf(const char *fmt, ...)
{
    // Only print in debug mode.
    if (!UciOptionFields.debug) return 0;

    va_list ap;
    va_start(ap, fmt);

    int result = vprintf(fmt, ap);

    fflush(stdout);
    va_end(ap);
    return result;
}

void uci_isready(const char *args __attribute__((unused)))
{
    puts("readyok");
    fflush(stdout);
}

void uci_quit(const char *args __attribute__((unused))) { wpool_stop(&SearchWorkerPool); }

void uci_stop(const char *args __attribute__((unused))) { wpool_stop(&SearchWorkerPool); }

void uci_ponderhit(const char *args __attribute__((unused))) { wpool_ponderhit(&SearchWorkerPool); }

void uci_uci(const char *args __attribute__((unused)))
{
    puts("id name Stash " UCI_VERSION);
    puts("id author Morgan Houppin");
    show_options(&UciOptionList);
    puts("uciok");
    fflush(stdout);
}

void uci_ucinewgame(const char *args)
{
    (void)args;

    // Wait for any unfinished search to complete.
    worker_wait_search_end(wpool_main_worker(&SearchWorkerPool));

    // Reset the TT contents.
    tt_bzero((size_t)UciOptionFields.threads);

    // Reset the histories for each worker.
    wpool_reset(&SearchWorkerPool);
}

void uci_debug(const char *args)
{
    char *copy = strdup(args ? args : "");

    if (copy == NULL) uci_allocation_failure("command copy");

    char *token = strtok(copy, Delimiters);

    UciOptionFields.debug = (strcmp(token, "on") == 0);
    free(copy);
}

// Pretty prints the board, along with the hash key and the eval.
void uci_d(const char *args __attribute__((unused)))
{
    const char *grid = "+---+---+---+---+---+---+---+---+";
    extern const char PieceIndexes[PIECE_NB];

    puts(grid);

    for (file_t rank = RANK_8; rank >= RANK_1; --rank)
    {
        for (file_t file = FILE_A; file <= FILE_H; ++file)
            printf("| %c ", PieceIndexes[piece_on(&UciBoard, create_sq(file, rank))]);

        puts("|");
        puts(grid);
    }

    printf("\nFEN: %s\nKey: 0x%" KEY_INFO "\n", board_fen(&UciBoard),
        (info_t)UciBoard.stack->boardKey);

    double eval = (double)evaluate(&UciBoard) / 100.0;

    printf(
        "Eval (from %s's POV): %+.2lf\n\n", UciBoard.sideToMove == WHITE ? "White" : "Black", eval);
    fflush(stdout);
}

void uci_position(const char *args)
{
    const char *StartPosFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    char *copy = strdup(args);

    if (copy == NULL) uci_allocation_failure("command copy");

    char *fen;
    char *ptr = copy;
    char *token = get_next_token(&ptr);

    if (!strcmp(token, "startpos"))
    {
        fen = strdup(StartPosFEN);

        if (fen == NULL) uci_allocation_failure("FEN string");

        token = get_next_token(&ptr);
    }
    else if (!strcmp(token, "fen"))
    {
        fen = strdup("");

        if (fen == NULL) uci_allocation_failure("FEN string");

        token = get_next_token(&ptr);

        while (token && strcmp(token, "moves"))
        {
            char *tmp = malloc(strlen(fen) + strlen(token) + 2);

            if (tmp == NULL) uci_allocation_failure("FEN string");

            strcpy(tmp, fen);
            strcat(tmp, " ");
            strcat(tmp, token);
            free(fen);
            fen = tmp;
            token = get_next_token(&ptr);
        }
    }
    else
    {
        free(copy);
        return;
    }

    free_boardstack(UciBoard.stack);

    Boardstack *firstStack = malloc(sizeof(Boardstack));

    if (firstStack == NULL) uci_allocation_failure("board stack");

    const int result = board_from_fen(&UciBoard, fen, UciOptionFields.chess960, firstStack);

    if (result < 0) board_from_fen(&UciBoard, StartPosFEN, UciOptionFields.chess960, firstStack);

    UciBoard.worker = wpool_main_worker(&SearchWorkerPool);
    free(fen);

    if (result >= 0)
    {
        token = get_next_token(&ptr);

        move_t move;
        size_t i = 1;

        while (token && (move = str_to_move(&UciBoard, token)) != NO_MOVE)
        {
            Boardstack *nextStack = malloc(sizeof(Boardstack));

            if (nextStack == NULL) uci_allocation_failure("board stack");

            do_move(&UciBoard, move, nextStack);
            token = get_next_token(&ptr);
            ++i;
        }

        if (token)
            debug_printf(
                "info string Failed to parse move token #%lu ('%s')\n", (unsigned long)i, token);
        debug_printf("info string Final board state: %s\n", board_fen(&UciBoard));
    }

    free(copy);
}

void uci_go(const char *args)
{
    worker_wait_search_end(wpool_main_worker(&SearchWorkerPool));

    memset(&UciSearchParams, 0, sizeof(SearchParams));
    list_all(&UciSearchMoves, &UciBoard);

    char *copy = strdup(args ? args : "");

    if (copy == NULL) uci_allocation_failure("command copy");

    char *token = strtok(copy, Delimiters);

    while (token)
    {
        if (strcmp(token, "searchmoves") == 0)
        {
            token = strtok(NULL, Delimiters);

            ExtendedMove *m = UciSearchMoves.moves;

            while (token)
            {
                (m++)->move = str_to_move(&UciBoard, token);
                token = strtok(NULL, Delimiters);
            }
            UciSearchMoves.last = m;
            break;
        }
        else if (strcmp(token, "wtime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.wtime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "btime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.btime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "winc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.winc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "binc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.binc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "movestogo") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.movestogo = atoi(token);
        }
        else if (strcmp(token, "depth") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.depth = atoi(token);
        }
        else if (strcmp(token, "nodes") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.nodes = (uint64_t)atoll(token);
        }
        else if (strcmp(token, "mate") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.mate = atoi(token);
        }
        else if (strcmp(token, "perft") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.perft = atoi(token);
        }
        else if (strcmp(token, "movetime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token) UciSearchParams.movetime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "infinite") == 0)
            UciSearchParams.infinite = 1;

        else if (strcmp(token, "ponder") == 0)
            UciSearchParams.ponder = 1;

        token = strtok(NULL, Delimiters);
    }

    wpool_start_search(&SearchWorkerPool, &UciBoard, &UciSearchParams);
    free(copy);
}

void uci_setoption(const char *args)
{
    if (!args) return;

    char *copy = strdup(args);

    if (copy == NULL) uci_allocation_failure("command copy");

    char *nameToken = strstr(copy, "name");
    char *valueToken = strstr(copy, "value");

    if (!nameToken)
    {
        free(copy);
        return;
    }

    char nameBuf[1024] = {0};
    char valueBuf[1024];

    if (valueToken)
    {
        *(valueToken - 1) = '\0';
        strcpy(valueBuf, valueToken + 6);

        // Remove the final newline to valueBuf.
        char *maybeNewline = &valueBuf[strlen(valueBuf) - 1];
        if (*maybeNewline == '\n') *maybeNewline = '\0';
    }
    else
        valueBuf[0] = '\0';

    char *token;

    token = strtok(nameToken + 4, Delimiters);
    while (token)
    {
        strcat(nameBuf, token);
        token = strtok(NULL, Delimiters);
        if (token) strcat(nameBuf, " ");
    }

    set_option(&UciOptionList, nameBuf, valueBuf);
    free(copy);
}

int execute_uci_cmd(const char *command)
{
    char *dup = strdup(command);

    if (dup == NULL) uci_allocation_failure("command string");

    char *cmd = strtok(dup, Delimiters);

    if (cmd == NULL)
    {
        free(dup);
        return 1;
    }

    for (size_t i = 0; UciCommands[i].commandName != NULL; ++i)
    {
        if (strcmp(UciCommands[i].commandName, cmd) == 0)
        {
            UciCommands[i].call(strtok(NULL, ""));
            break;
        }
    }

    int keepLooping = !!strcmp(cmd, "quit");

    free(dup);
    return keepLooping;
}

void on_hash_set(void *data)
{
    // Wait for any unfinished search to complete.
    worker_wait_search_end(wpool_main_worker(&SearchWorkerPool));
    tt_resize((size_t) * (long *)data);
}

void on_clear_hash(void *unused)
{
    (void)unused;
    // The "Clear Hash" option does exactly the same thing as the "ucinewgame" command.
    uci_ucinewgame(NULL);
}

void on_thread_set(void *data)
{
    // Wait for any unfinished search to complete.
    worker_wait_search_end(wpool_main_worker(&SearchWorkerPool));
    wpool_init(&SearchWorkerPool, (unsigned long)*(long *)data);
}

void uci_loop(int argc, char **argv)
{
    init_option_list(&UciOptionList);
    add_option_spin_int(
        &UciOptionList, "Threads", &UciOptionFields.threads, 1, 256, &on_thread_set);
    add_option_spin_int(&UciOptionList, "Hash", &UciOptionFields.hash, 1, MAX_HASH, &on_hash_set);
    add_option_spin_int(
        &UciOptionList, "Move Overhead", &UciOptionFields.moveOverhead, 0, 30000, NULL);
    add_option_spin_int(&UciOptionList, "MultiPV", &UciOptionFields.multiPv, 1, 500, NULL);
    add_option_check(&UciOptionList, "UCI_Chess960", &UciOptionFields.chess960, NULL);
    add_option_check(&UciOptionList, "UCI_ShowWDL", &UciOptionFields.showWDL, NULL);
    add_option_check(&UciOptionList, "NormalizeScore", &UciOptionFields.normalizeScore, NULL);
    add_option_check(&UciOptionList, "Ponder", &UciOptionFields.ponder, NULL);
    add_option_button(&UciOptionList, "Clear Hash", &on_clear_hash);

    uci_position("startpos");

    if (argc > 1)
        for (int i = 1; i < argc; ++i) execute_uci_cmd(argv[i]);
    else
    {
        char *line = malloc(16384);

        while (fgets(line, 16384, stdin) != NULL)
            if (execute_uci_cmd(line) == 0) break;

        free(line);
    }

    uci_quit(NULL);
    quit_option_list(&UciOptionList);
}
