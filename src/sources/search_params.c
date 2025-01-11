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

#include "search_params.h"

#include <string.h>

#include "chess_types.h"
#include "syncio.h"

void search_params_init(
    SearchParams *search_params,
    i64 move_overhead,
    i64 multi_pv,
    bool show_wdl,
    bool normalize_score
) {
    *search_params = (SearchParams) {
        .move_overhead = move_overhead,
        .multi_pv = multi_pv,
        .show_wdl = show_wdl,
        .normalize_score = normalize_score,

        .wtime = 0,
        .btime = 0,
        .winc = 0,
        .binc = 0,
        .tc_is_set = false,

        .depth = 0,
        .nodes = 0,
        .movetime = 0,
        .mate = 0,
        .perft = 0,
        .infinite = false,
        .ponder = false,
    };

    search_params->searchmoves.end = search_params->searchmoves.moves;
}

static bool
    try_set_duration(StringView token, StringView *command_args, Duration *field, StringView name) {
    if (!strview_equals_strview(token, name)) {
        return false;
    }

    i64 value;

    token = strview_next_word(command_args);

    if (!strview_parse_i64(token, &value)) {
        info_debug(
            "info string Warning: unable to parse '%.*s' as a value for '%.*s', so this parameter "
            "will be ignored\n",
            (int)token.size,
            (const char *)token.data,
            (int)name.size,
            (const char *)name.data
        );
    } else {
        *field = (Duration)value;
    }

    return true;
}

static bool try_set_u16(
    StringView token,
    StringView *command_args,
    u16 *field,
    u16 min_value,
    u16 max_value,
    StringView name
) {
    if (!strview_equals_strview(token, name)) {
        return false;
    }

    u64 value;

    token = strview_next_word(command_args);

    if (!strview_parse_u64(token, &value)) {
        info_debug(
            "info string Warning: unable to parse '%.*s' as a value for '%.*s', so this parameter "
            "will be ignored\n",
            (int)token.size,
            (const char *)token.data,
            (int)name.size,
            (const char *)name.data
        );
    } else if (value < (u64)min_value || value > (u64)max_value) {
        info_debug(
            "info string Warning: " FORMAT_LARGE_INT
            " is outside the supported range (%u, %u) for '%.*s', so we will clamp the value\n",
            (LargeInt)value,
            min_value,
            max_value,
            (int)name.size,
            (const char *)name.data
        );
        *field = (u16)u64_clamp(value, min_value, max_value);
    } else {
        *field = (u16)value;
    }

    return true;
}

static bool try_set_u64(
    StringView token,
    StringView *command_args,
    u64 *field,
    u64 min_value,
    u64 max_value,
    StringView name
) {
    if (!strview_equals_strview(token, name)) {
        return false;
    }

    u64 value;

    token = strview_next_word(command_args);

    if (!strview_parse_u64(token, &value)) {
        info_debug(
            "info string Warning: unable to parse '%.*s' as a value for '%.*s', so this parameter "
            "will be ignored\n",
            (int)token.size,
            (const char *)token.data,
            (int)name.size,
            (const char *)name.data
        );
    } else if (value < min_value || value > max_value) {
        info_debug(
            "info string Warning: " FORMAT_LARGE_INT
            " is outside the supported range (" FORMAT_LARGE_INT ", " FORMAT_LARGE_INT
            ") for '%.*s', so we will clamp the value\n",
            (LargeInt)value,
            (LargeInt)min_value,
            (LargeInt)max_value,
            (int)name.size,
            (const char *)name.data
        );
        *field = u64_clamp(value, min_value, max_value);
    } else {
        *field = value;
    }

    return true;
}

static bool try_set_bool(StringView token, bool *field, StringView name) {
    if (!strview_equals_strview(token, name)) {
        return false;
    }

    *field = true;
    return true;
}

static bool try_set_searchmoves(
    StringView token,
    StringView *command_args,
    Movelist *restrict movelist,
    const Board *restrict root_board
) {
    if (!strview_equals_strview(token, STATIC_STRVIEW("searchmoves"))) {
        return false;
    }

    while (true) {
        token = strview_next_word(command_args);

        if (token.size == 0) {
            break;
        }

        Move move = board_uci_to_move(root_board, token);

        if (move != NO_MOVE && !movelist_contains(movelist, move)) {
            (movelist->end++)->move = move;
        } else if (move == NO_MOVE) {
            info_debug(
                "info string Warning: '%.*s' is not a valid/legal UCI move for the given position, "
                "so we will ignore it\n",
                (int)token.size,
                (const char *)token.data
            );
        } else {
            info_debug(
                "info string Warning: '%.*s' has been specified twice\n",
                (int)token.size,
                (const char *)token.data
            );
        }
    }

    return true;
}

void search_params_set_from_uci(
    SearchParams *restrict search_params,
    const Board *restrict root_board,
    StringView command_args
) {
    while (true) {
        StringView token = strview_next_word(&command_args);

        if (token.size == 0) {
            break;
        }

        if (try_set_duration(
                token,
                &command_args,
                &search_params->wtime,
                STATIC_STRVIEW("wtime")
            )) {
            search_params->tc_is_set = true;
            continue;
        }

        if (try_set_duration(
                token,
                &command_args,
                &search_params->btime,
                STATIC_STRVIEW("btime")
            )) {
            search_params->tc_is_set = true;
            continue;
        }

        if (try_set_duration(token, &command_args, &search_params->winc, STATIC_STRVIEW("winc"))) {
            search_params->tc_is_set = true;
            continue;
        }

        if (try_set_duration(token, &command_args, &search_params->binc, STATIC_STRVIEW("binc"))) {
            search_params->tc_is_set = true;
            continue;
        }

        if (try_set_u16(
                token,
                &command_args,
                &search_params->movestogo,
                0,
                4096,
                STATIC_STRVIEW("movestogo")
            )) {
            search_params->tc_is_set = true;
            continue;
        }

        if (try_set_u16(
                token,
                &command_args,
                &search_params->depth,
                0,
                MAX_PLIES,
                STATIC_STRVIEW("depth")
            )) {
            continue;
        }

        if (try_set_u64(
                token,
                &command_args,
                &search_params->nodes,
                0,
                UINT64_MAX,
                STATIC_STRVIEW("nodes")
            )) {
            continue;
        }

        if (try_set_duration(
                token,
                &command_args,
                &search_params->movetime,
                STATIC_STRVIEW("movetime")
            )) {
            continue;
        }

        if (try_set_u16(
                token,
                &command_args,
                &search_params->mate,
                0,
                MAX_PLIES / 2,
                STATIC_STRVIEW("mate")
            )) {
            continue;
        }

        if (try_set_u16(
                token,
                &command_args,
                &search_params->perft,
                0,
                MAX_PLIES,
                STATIC_STRVIEW("perft")
            )) {
            continue;
        }

        if (try_set_bool(token, &search_params->infinite, STATIC_STRVIEW("infinite"))) {
            continue;
        }

        if (try_set_bool(token, &search_params->ponder, STATIC_STRVIEW("ponder"))) {
            continue;
        }

        // "searchmoves" is always the last search parameter of the "go" command.
        if (try_set_searchmoves(token, &command_args, &search_params->searchmoves, root_board)) {
            break;
        }

        info_debug(
            "info string Warning: unrecognized go parameter '%.*s', so we will ignore it\n",
            (int)token.size,
            (const char *)token.data
        );
    }

    if (movelist_size(&search_params->searchmoves) == 0) {
        movelist_generate_legal(&search_params->searchmoves, root_board);
    }
}

void search_params_copy(SearchParams *restrict search_params, const SearchParams *restrict other) {
    *search_params = *other;
    search_params->searchmoves.end =
        &search_params->searchmoves.moves[movelist_size(&other->searchmoves)];
}
