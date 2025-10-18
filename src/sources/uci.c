/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2025 Morgan Houppin
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
#include "search_params.h"
#include "strmanip.h"
#include "syncio.h"
#include "wdl.h"
#include "wmalloc.h"

#define UCI_VERSION "v37.15"

static const Command UciCommands[] = {
    {STATIC_STRVIEW("bench"), uci_bench},
    {STATIC_STRVIEW("d"), uci_d},
    {STATIC_STRVIEW("debug"), uci_debug},
    {STATIC_STRVIEW("go"), uci_go},
    {STATIC_STRVIEW("isready"), uci_isready},
    {STATIC_STRVIEW("ponderhit"), uci_ponderhit},
    {STATIC_STRVIEW("position"), uci_position},
    {STATIC_STRVIEW("quit"), uci_quit},
    {STATIC_STRVIEW("setoption"), uci_setoption},
    {STATIC_STRVIEW("stop"), uci_stop},
    {STATIC_STRVIEW("t"), uci_t},
    {STATIC_STRVIEW("uci"), uci_uci},
    {STATIC_STRVIEW("ucinewgame"), uci_ucinewgame},
};

void on_threads_change(__attribute__((unused)) const OptionParams *params, void *uci_ptr) {
    Uci *uci = (Uci *)uci_ptr;
    wpool_resize(&uci->worker_pool, (u64)uci->option_values.threads);
}

void on_hash_change(__attribute__((unused)) const OptionParams *params, void *uci_ptr) {
    Uci *uci = (Uci *)uci_ptr;
    tt_resize(&uci->worker_pool.tt, (u64)uci->option_values.hash, (u64)uci->option_values.threads);
}

void on_clear_hash(__attribute__((unused)) const OptionParams *params, void *uci_ptr) {
    uci_ucinewgame((Uci *)uci_ptr, EmptyStrview);
}

static void uci_init_options(Uci *uci) {
    uci->option_values = (OptionValues) {
        .threads = 1,
        .hash = 1,
        .move_overhead = 30,
        .multi_pv = 1,
        .chess960 = false,
        .ponder = false,
        .show_wdl = false,
        .normalize_score = true,
        .tm_for_nodes = false,
    };

    optlist_init(&uci->option_list);
    optlist_add_spin_integer(
        &uci->option_list,
        strview_from_cstr("Threads"),
        &uci->option_values.threads,
        1,
        512,
        false,
        on_threads_change,
        (void *)uci
    );
    optlist_add_spin_integer(
        &uci->option_list,
        strview_from_cstr("Hash"),
        &uci->option_values.hash,
        1,
        33554432,
        false,
        on_hash_change,
        (void *)uci
    );
    optlist_add_spin_integer(
        &uci->option_list,
        strview_from_cstr("MoveOverhead"),
        &uci->option_values.move_overhead,
        1,
        5000,
        false,
        NULL,
        NULL
    );
    optlist_add_spin_integer(
        &uci->option_list,
        strview_from_cstr("MultiPV"),
        &uci->option_values.multi_pv,
        1,
        500,
        false,
        NULL,
        NULL
    );
    optlist_add_check(
        &uci->option_list,
        strview_from_cstr("UCI_Chess960"),
        &uci->option_values.chess960,
        NULL,
        NULL
    );
    optlist_add_check(
        &uci->option_list,
        strview_from_cstr("UCI_ShowWDL"),
        &uci->option_values.show_wdl,
        NULL,
        NULL
    );
    optlist_add_check(
        &uci->option_list,
        strview_from_cstr("NormalizeScore"),
        &uci->option_values.normalize_score,
        NULL,
        NULL
    );
    optlist_add_check(
        &uci->option_list,
        strview_from_cstr("TimemanForNodes"),
        &uci->option_values.tm_for_nodes,
        NULL,
        NULL
    );
    optlist_add_check(
        &uci->option_list,
        strview_from_cstr("Ponder"),
        &uci->option_values.ponder,
        NULL,
        NULL
    );
    optlist_add_button(
        &uci->option_list,
        strview_from_cstr("Clear Hash"),
        &on_clear_hash,
        (void *)uci
    );
}

void uci_init(Uci *uci) {
    Boardstack *stack = wrap_malloc(sizeof(Boardstack));

    uci_init_options(uci);
    board_try_init(&uci->root_board, StartposStr, false, stack);
    wpool_init(&uci->worker_pool);
}

void uci_destroy(Uci *uci) {
    wpool_destroy(&uci->worker_pool);
    optlist_destroy(&uci->option_list);
    boardstack_destroy(uci->root_board.stack);
}

void uci_d(Uci *uci, __attribute__((unused)) StringView args) {
    StringView grid = STATIC_STRVIEW("+---+---+---+---+---+---+---+---+\n");

    sync_lock_stdout();
    fwrite_strview(stdout, grid);

    for (Rank rank = RANK_8; rank_is_valid(rank); --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) {
            fwrite_strview(stdout, STATIC_STRVIEW("| "));
            fputc(
                PieceIndexes.data[board_piece_on(&uci->root_board, create_square(file, rank))],
                stdout
            );
            fputc(' ', stdout);
        }

        fwrite_strview(stdout, STATIC_STRVIEW("|\n"));
        fwrite_strview(stdout, grid);
    }

    Score eval = evaluate(&uci->root_board);

    if (uci->option_values.normalize_score) {
        eval = normalized_score(eval);
    }

    fwrite_strview(stdout, STATIC_STRVIEW("\nFEN: "));
    fwrite_strview(stdout, board_get_fen(&uci->root_board));
    fprintf(
        stdout,
        "\nKey: 0x%" PRIx64 "\nEval (from %s's POV): %+.2lf\n\n",
        uci->root_board.stack->board_key,
        uci->root_board.side_to_move == WHITE ? "White" : "Black",
        (double)eval / 100.0
    );
    fflush(stdout);
    sync_unlock_stdout();
}

void uci_debug(Uci *uci __attribute__((unused)), StringView args) {
    StringView token = strview_next_word(&args);
    bool debug_state = strview_equals_strview(token, STATIC_STRVIEW("on"));

    toggle_debug(debug_state);
}

void uci_go(Uci *uci, StringView args) {
    SearchParams search_params;

    search_params_init(
        &search_params,
        uci->option_values.move_overhead,
        uci->option_values.multi_pv,
        uci->option_values.show_wdl,
        uci->option_values.normalize_score,
        uci->option_values.tm_for_nodes
    );
    search_params_set_from_uci(&search_params, &uci->root_board, args);
    wpool_start_search(&uci->worker_pool, &uci->root_board, &search_params);
}

void uci_isready(__attribute__((unused)) Uci *uci, __attribute__((unused)) StringView args) {
    puts("readyok");
    fflush(stdout);
}

void uci_ponderhit(Uci *uci, __attribute__((unused)) StringView args) {
    wpool_ponderhit(&uci->worker_pool);
}

void uci_position(Uci *uci, StringView args) {
    StringView token = strview_next_word(&args);
    StringView fen;
    Boardstack *stack;

    if (strview_equals_strview(token, STATIC_STRVIEW("startpos"))) {
        fen = StartposStr;
    } else if (strview_equals_strview(token, STATIC_STRVIEW("fen"))) {
        usize moves_idx = strview_find_strview(args, STATIC_STRVIEW("moves"));

        if (moves_idx == NPOS) {
            moves_idx = args.size;
        }

        fen = strview_subview(args, 0, moves_idx);
        args = strview_subview(args, moves_idx, args.size);
    } else {
        info_debug(
            "info string Error: unrecognized position type '%.*s'\n",
            (int)token.size,
            (const char *)token.data
        );
        return;
    }

    boardstack_destroy(uci->root_board.stack);
    stack = wrap_malloc(sizeof(Boardstack));

    if (!board_try_init(&uci->root_board, fen, uci->option_values.chess960, stack)) {
        board_try_init(&uci->root_board, StartposStr, uci->option_values.chess960, stack);
        return;
    }

    token = strview_next_word(&args);

    if (token.size == 0) {
        return;
    }

    if (!strview_equals_strview(token, STATIC_STRVIEW("moves"))) {
        info_debug(
            "info string Error: unrecognized token, expected 'moves', got '%.*s'\n",
            (int)token.size,
            (const char *)token.data
        );
        return;
    }

    while (true) {
        token = strview_next_word(&args);

        if (token.size == 0) {
            break;
        }

        Move move = board_uci_to_move(&uci->root_board, token);

        if (move == NO_MOVE) {
            info_debug(
                "info string Error: invalid/illegal move '%.*s', position parsing will stop here\n",
                (int)token.size,
                (const char *)token.data
            );
            break;
        }

        stack = wrap_malloc(sizeof(Boardstack));
        board_do_move(&uci->root_board, move, stack);
    }

    StringView new_fen = board_get_fen(&uci->root_board);

    info_debug(
        "info string Final position state: '%.*s'\n",
        (int)new_fen.size,
        (const char *)new_fen.data
    );
}

void uci_quit(Uci *uci, __attribute__((unused)) StringView args) {
    wpool_stop(&uci->worker_pool);
}

void uci_setoption(Uci *uci, StringView args) {
    wpool_wait_search_completion(&uci->worker_pool);

    StringView token = strview_next_word(&args);
    String name;

    if (!strview_equals_strview(token, STATIC_STRVIEW("name"))) {
        info_debug(
            "info string Error: unrecognized setoption token, expected 'name', got '%.*s'\n",
            (int)token.size,
            (const char *)token.data
        );
        return;
    }

    // There must be at least one word in an option name. If we get the "value" field here already,
    // the `setoption` command is badly formatted.
    string_init_from_strview(&name, strview_next_word(&args));

    while (true) {
        token = strview_next_word(&args);

        if (token.size == 0 || strview_equals_strview(token, STATIC_STRVIEW("value"))) {
            break;
        }

        string_push_back(&name, ' ');
        string_push_back_strview(&name, token);
    }

    optlist_set_option(
        &uci->option_list,
        strview_from_string(&name),
        strview_trim_whitespaces(args)
    );
    string_destroy(&name);
}

void uci_stop(Uci *uci, __attribute__((unused)) StringView args) {
    wpool_stop(&uci->worker_pool);
}

void uci_t(Uci *uci, __attribute__((unused)) StringView args) {
    optlist_show_tunable_options(&uci->option_list);
}

void uci_uci(Uci *uci, __attribute__((unused)) StringView args) {
    puts("id name Stash " UCI_VERSION);
    puts("id author Morgan Houppin et al. (see AUTHORS file)");
    optlist_show_options(&uci->option_list);
    puts("uciok");
    fflush(stdout);
}

void uci_ucinewgame(Uci *uci, __attribute__((unused)) StringView args) {
    wpool_wait_search_completion(&uci->worker_pool);
    wpool_init_new_game(&uci->worker_pool);
}

bool uci_exec_command(Uci *uci, StringView command) {
    StringView command_name = strview_next_word(&command);
    bool found = false;

    for (usize i = 0; i < sizeof(UciCommands) / sizeof(UciCommands[0]); ++i) {
        if (strview_equals_strview(command_name, UciCommands[i].cmd_name)) {
            UciCommands[i].cmd_exec(uci, command);
            found = true;
            break;
        }
    }

    if (!found) {
        info_debug(
            "info string Error: unknown command '%.*s'\n",
            (int)command_name.size,
            (const char *)command_name.data
        );
    }

    return !strview_equals_strview(command_name, STATIC_STRVIEW("quit"));
}

void uci_loop(int argc, char **argv) {
    Uci uci;

    uci_init(&uci);

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            uci_exec_command(&uci, strview_from_cstr(argv[i]));
        }
    } else {
        String line;

        string_init(&line);

        while (string_getline(stdin, &line)) {
            if (!uci_exec_command(&uci, strview_from_string(&line))) {
                break;
            }
        }

        string_destroy(&line);
    }

    uci_quit(&uci, EmptyStrview);
    wpool_wait_search_completion(&uci.worker_pool);
    uci_destroy(&uci);
}
