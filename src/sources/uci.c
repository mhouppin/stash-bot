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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "engine.h"
#include "imath.h"
#include "lazy_smp.h"
#include "option.h"
#include "tt.h"
#include "uci.h"

#define UCI_VERSION "v32.13"

const cmdlink_t commands[] =
{
    {"bench", &uci_bench},
    {"d", &uci_d},
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

const char *BoundStr[] = {
    "",
    " upperbound",
    " lowerbound",
    ""
};

// Finds the next token in the given string, writes a nullbyte to its end,
// and returns it (after incrementing the string pointer).
// This is mainly used as a replacement to strtok_r(), which isn't available
// on MinGW.

char *get_next_token(char **str)
{
    while (isspace(**str) && **str != '\0')
        ++(*str);

    if (**str == '\0')
        return (NULL);

    char *retval = *str;

    while (!isspace(**str) && **str != '\0')
        ++(*str);

    if (**str == '\0')
        return (retval);

    **str = '\0';
    ++(*str);
    return (retval);
}

void wait_search_end(void)
{
    pthread_mutex_lock(&EngineMutex);
    while (EngineMode != WAITING)
        pthread_cond_wait(&EngineCond, &EngineMutex);
    pthread_mutex_unlock(&EngineMutex);
}

const char *move_to_str(move_t move, bool isChess960)
{
    static char buf[6];

    if (move == NO_MOVE)
        return ("none");

    if (move == NULL_MOVE)
        return ("0000");

    square_t from = from_sq(move), to = to_sq(move);

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

    return (buf);
}

move_t str_to_move(const board_t *board, const char *str)
{
    char *trick = strdup(str);

    if (strlen(str) == 5)
        trick[4] = tolower(trick[4]);

    movelist_t movelist;

    list_all(&movelist, board);

    for (const extmove_t *m = movelist_begin(&movelist); m < movelist_end(&movelist); ++m)
    {
        const char *s = move_to_str(m->move, board->chess960);
        if (!strcmp(trick, s))
        {
            free(trick);
            return (m->move);
        }
    }

    free(trick);
    return (NO_MOVE);
}

const char *score_to_str(score_t score)
{
    static char buf[12];

    if (abs(score) >= MATE_FOUND)
        sprintf(buf, "mate %d", (score > 0 ? MATE - score + 1 : -MATE - score) / 2);
    else
        sprintf(buf, "cp %d", score);

    return (buf);
}

void print_pv(const board_t *board, root_move_t *rootMove, int multiPv, int depth, clock_t time, int bound)
{
    uint64_t nodes = get_node_count();
    uint64_t nps = nodes / (time + !time) * 1000;
    bool searchedMove = (rootMove->score != -INF_SCORE);
    score_t rootScore = (searchedMove) ? rootMove->score : rootMove->prevScore;

    printf("info depth %d seldepth %d multipv %d score %s%s", max(depth + searchedMove, 1),
        rootMove->seldepth, multiPv, score_to_str(rootScore), BoundStr[bound]);
    printf(" nodes %" FMT_INFO " nps %" FMT_INFO " hashfull %d time %" FMT_INFO " pv",
        (info_t)nodes, (info_t)nps, tt_hashfull(), (info_t)time);

    for (size_t k = 0; rootMove->pv[k]; ++k)
        printf(" %s", move_to_str(rootMove->pv[k], board->chess960));
    putchar('\n');
}

void uci_isready(const char *args __attribute__((unused)))
{
    puts("readyok");
    fflush(stdout);
}

void uci_quit(const char *args __attribute__((unused)))
{
    pthread_mutex_lock(&EngineMutex);
    EngineSend = DO_ABORT;
    pthread_mutex_unlock(&EngineMutex);
    pthread_cond_signal(&EngineCond);
}

void uci_stop(const char *args __attribute__((unused)))
{
    pthread_mutex_lock(&EngineMutex);
    EngineSend = DO_EXIT;
    pthread_mutex_unlock(&EngineMutex);
    pthread_cond_signal(&EngineCond);
}

void uci_ponderhit(const char *args __attribute__((unused)))
{
    EnginePonderhit = 1;
}

void uci_uci(const char *args __attribute__((unused)))
{
    puts("id name Stash " UCI_VERSION);
    puts("id author Morgan Houppin");
    show_options(&OptionList);
    puts("uciok");
    fflush(stdout);
}

void uci_ucinewgame(const char *args)
{
    (void)args;
    wait_search_end();
    tt_bzero((size_t)Options.threads);
    wpool_reset();
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
            printf("| %c ", PieceIndexes[piece_on(&Board, create_sq(file, rank))]);

        puts("|");
        puts(grid);
    }

    printf("\nFEN: %s\nKey: 0x%" KEY_INFO "\n", board_fen(&Board), (info_t)Board.stack->boardKey);

    double eval = (double)evaluate(&Board) / 100.0;

    printf("Eval (from %s's POV): %+.2lf\n\n", Board.sideToMove == WHITE ? "White" : "Black", eval);
    fflush(stdout);
}

void uci_position(const char *args)
{
    static boardstack_t **hiddenList = NULL;
    static size_t hiddenSize = 0;

    wait_search_end();

    if (hiddenSize > 0)
    {
        for (size_t i = 0; i < hiddenSize; ++i)
            free(hiddenList[i]);
        free(hiddenList);
    }

    hiddenList = malloc(sizeof(boardstack_t *));
    *hiddenList = malloc(sizeof(boardstack_t));
    hiddenSize = 1;

    char *fen;
    char *copy = strdup(args);
    char *ptr = copy;
    char *token = get_next_token(&ptr);

    if (!strcmp(token, "startpos"))
    {
        fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        token = get_next_token(&ptr);
    }
    else if (!strcmp(token, "fen"))
    {
        fen = strdup("");
        token = get_next_token(&ptr);

        while (token && strcmp(token, "moves"))
        {
            char *tmp = malloc(strlen(fen) + strlen(token) + 2);

            strcpy(tmp, fen);
            strcat(tmp, " ");
            strcat(tmp, token);
            free(fen);
            fen = tmp;
            token = get_next_token(&ptr);
        }
    }
    else
        return ;

    set_board(&Board, fen, Options.chess960, *hiddenList);
    Board.worker = WPool.list;
    free(fen);
    token = get_next_token(&ptr);

    move_t move;

    while (token && (move = str_to_move(&Board, token)) != NO_MOVE)
    {
        hiddenList = realloc(hiddenList, sizeof(boardstack_t *) * ++hiddenSize);
        hiddenList[hiddenSize - 1] = malloc(sizeof(boardstack_t));

        do_move(&Board, move, hiddenList[hiddenSize - 1]);
        token = get_next_token(&ptr);
    }

    free(copy);
}

void uci_go(const char *args)
{
    wait_search_end();
    pthread_mutex_lock(&EngineMutex);

    EngineSend = DO_THINK;
    EnginePonderhit = 0;
    memset(&SearchParams, 0, sizeof(goparams_t));
    list_all(&SearchMoves, &Board);

    char *copy = strdup(args ? args : "");
    char *token = strtok(copy, Delimiters);

    while (token)
    {
        if (strcmp(token, "searchmoves") == 0)
        {
            token = strtok(NULL, Delimiters);

            extmove_t *m = SearchMoves.moves;

            while (token)
            {
                (m++)->move = str_to_move(&Board, token);
                token = strtok(NULL, Delimiters);
            }
            SearchMoves.last = m;
            break ;
        }
        else if (strcmp(token, "wtime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.wtime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "btime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.btime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "winc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.winc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "binc") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.binc = (clock_t)atoll(token);
        }
        else if (strcmp(token, "movestogo") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.movestogo = atoi(token);
        }
        else if (strcmp(token, "depth") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.depth = atoi(token);
        }
        else if (strcmp(token, "nodes") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.nodes = (size_t)atoll(token);
        }
        else if (strcmp(token, "mate") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.mate = atoi(token);
        }
        else if (strcmp(token, "perft") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.perft = atoi(token);
        }
        else if (strcmp(token, "movetime") == 0)
        {
            token = strtok(NULL, Delimiters);
            if (token)
                SearchParams.movetime = (clock_t)atoll(token);
        }
        else if (strcmp(token, "infinite") == 0)
            SearchParams.infinite = 1;

        else if (strcmp(token, "ponder") == 0)
            SearchParams.ponder = 1;

        token = strtok(NULL, Delimiters);
    }

    EngineMode = THINKING;

    pthread_cond_broadcast(&EngineCond);
    pthread_mutex_unlock(&EngineMutex);
    free(copy);
}

void uci_setoption(const char *args)
{
    if (!args)
        return ;

    char *copy = strdup(args);
    char *nameToken = strstr(copy, "name");
    char *valueToken = strstr(copy, "value");

    if (!nameToken)
    {
        free(copy);
        return ;
    }

    char nameBuf[1024] = {0};
    char valueBuf[1024];

    if (valueToken)
    {
        *(valueToken - 1) = '\0';
        strcpy(valueBuf, valueToken + 6);

        // Remove the final newline to valueBuf.

        char *maybeNewline = &valueBuf[strlen(valueBuf) - 1];
        if (*maybeNewline == '\n')
            *maybeNewline = '\0';
    }
    else
        valueBuf[0] = '\0';

    char *token;

    token = strtok(nameToken + 4, Delimiters);
    while (token)
    {
        strcat(nameBuf, token);
        token = strtok(NULL, Delimiters);
        if (token)
            strcat(nameBuf, " ");
    }

    set_option(&OptionList, nameBuf, valueBuf);
    free(copy);
}

int execute_uci_cmd(const char *command)
{
    char *dup = strdup(command);
    char *cmd = strtok(dup, Delimiters);

    if (!cmd)
    {
        free(dup);
        return (1);
    }

    for (size_t i = 0; commands[i].commandName != NULL; ++i)
    {
        if (strcmp(commands[i].commandName, cmd) == 0)
        {
            commands[i].call(strtok(NULL, ""));
            break ;
        }
    }

    if (strcmp(cmd, "quit") == 0)
    {
        free(dup);
        return (0);
    }

    free(dup);
    return (1);
}

void on_hash_set(void *data)
{
    tt_resize((size_t)*(long *)data);
    printf("info string set Hash to %lu MB\n", *(long *)data);
    fflush(stdout);
}

void on_clear_hash(void *nothing __attribute__((unused)))
{
    tt_bzero((size_t)Options.threads);
    puts("info string cleared hash");
    fflush(stdout);
}

void on_thread_set(void *data)
{
    wpool_init((int)*(long *)data);
    printf("info string set Threads to %lu\n", *(long *)data);
    fflush(stdout);
}

void uci_loop(int argc, char **argv)
{
    init_option_list(&OptionList);
    add_option_spin_int(&OptionList, "Threads", &Options.threads, 1, 256, &on_thread_set);
    add_option_spin_int(&OptionList, "Hash", &Options.hash, 1, MAX_HASH, &on_hash_set);
    add_option_spin_int(&OptionList, "Move Overhead", &Options.moveOverhead, 0, 30000, NULL);
    add_option_spin_int(&OptionList, "MultiPV", &Options.multiPv, 1, 500, NULL);
    add_option_check(&OptionList, "UCI_Chess960", &Options.chess960, NULL);
    add_option_check(&OptionList, "Ponder", &Options.ponder, NULL);
    add_option_button(&OptionList, "Clear Hash", &on_clear_hash);

    uci_position("startpos");

    if (argc > 1)
        for (int i = 1; i < argc; ++i)
            execute_uci_cmd(argv[i]);
    else
    {
        char *line = malloc(16384);

        while (fgets(line, 16384, stdin) != NULL)
            if (execute_uci_cmd(line) == 0)
                break ;

        free(line);
    }

    wait_search_end();
    uci_quit(NULL);
    quit_option_list(&OptionList);
}
