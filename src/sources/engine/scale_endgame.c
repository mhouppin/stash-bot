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

#include <stdlib.h>
#include "engine.h"
#include "imath.h"

bool    ocb_endgame(const board_t *board)
{
    bitboard_t  wbishop = piece_bb(board, WHITE, BISHOP);

    if (!wbishop || more_than_one(wbishop))
        return (false);

    bitboard_t  bbishop = piece_bb(board, BLACK, BISHOP);

    if (!bbishop || more_than_one(bbishop))
        return (false);

    bitboard_t  dsq_mask = (wbishop | bbishop) & DARK_SQUARES;

    return (!!dsq_mask && !more_than_one(dsq_mask));
}

score_t scale_endgame(const board_t *board, score_t eg)
{
    // Only detect scalable endgames from the side with a positive evaluation.
    // This allows us to quickly filter out positions which shouldn't be scaled,
    // even though they have a theoretical scaling factor in our code (like KNvKPPPP).

    color_t     strong_side = (eg > 0) ? WHITE : BLACK;
    color_t     weak_side = not_color(strong_side);
    int         factor;
    score_t     strong_mat = board->stack->material[strong_side];
    score_t     weak_mat = board->stack->material[weak_side];
    bitboard_t  strong_pawns = piece_bb(board, strong_side, PAWN);
    bitboard_t  weak_pawns = piece_bb(board, weak_side, PAWN);

    // No pawns and low material difference, the endgame is either drawn
    // or very difficult to win.
    if (!strong_pawns && strong_mat - weak_mat <= BISHOP_MG_SCORE)
    {
        if (strong_mat <= BISHOP_MG_SCORE)
            factor = 0;
        else
            factor = clamp((strong_mat - weak_mat) / 8, 8, 32);
    }

    // OCB endgames: scale based on the number of remaining pieces of the strong side.
    else if (ocb_endgame(board))
        factor = 36 + popcount(color_bb(board, strong_side)) * 6;

    // Rook endgames: drawish if the pawn advantage is small, and all strong side pawns
    // are on the same side of the board. Don't scale if the defending king is far from
    // his own pawns.
    else if (strong_mat == ROOK_MG_SCORE && weak_mat == ROOK_MG_SCORE
        && (popcount(strong_pawns) - popcount(weak_pawns) < 2)
        && !!(KINGSIDE_BITS & strong_pawns) != !!(QUEENSIDE_BITS & strong_pawns)
        && (king_moves(get_king_square(board, weak_side)) & weak_pawns))
        factor = 64;

    // Other endgames. Decrease the endgame score as the number of pawns of the strong
    // side gets lower.
    else
        factor = min(128, 96 + 8 * popcount(strong_pawns));

    eg = (score_t)((int32_t)eg * factor / 128);

    return (eg);
}
