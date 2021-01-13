/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

    if (popcount(wbishop) != 1)
        return (false);

    bitboard_t  bbishop = piece_bb(board, BLACK, BISHOP);

    if (popcount(bbishop) != 1)
        return (false);

    square_t    wbsq = first_square(wbishop);
    square_t    bbsq = first_square(bbishop);

    return ((wbsq + rank_of_square(wbsq) + bbsq + rank_of_square(bbsq)) & 1);
}

score_t scale_endgame(const board_t *board, score_t eg)
{
    // Only detect scalable endgames from the side with a positive evaluation.
    // This allows us to quickly filter out positions which shouldn't be scaled,
    // even though they have a theoretical scaling factor in our code (like KNvKPPPP).

    color_t strong_side = (eg > 0) ? WHITE : BLACK;
    color_t weak_side = not_color(strong_side);
    int     factor;

    score_t     strong_mat = board->stack->material[strong_side];
    score_t     weak_mat = board->stack->material[weak_side];
    bitboard_t  strong_pawns = piece_bb(board, strong_side, PAWN);
    bitboard_t  weak_pawns = piece_bb(board, weak_side, PAWN);

    // Zero/one minor + no pawns, the endgame is unwinnable
    if (strong_mat <= BISHOP_MG_SCORE && !strong_pawns)
        factor = 0;

    // Two knights + no pawns, the endgame is very drawish
    else if (strong_mat == KNIGHT_MG_SCORE * 2 && !strong_pawns)
    {
        // If the losing side has no pawns, it is likely impossible to force a mate.

        factor = !weak_pawns ? 0 : 16;
    }

    // No pawns and weak side has pieces, scale based on the remaining material
    else if (!strong_pawns && weak_mat)
    {
        score_t s = strong_mat, w = weak_mat + popcount(weak_pawns) * PAWN_MG_SCORE;
        factor = clamp((s + s - w) / 25, 16, 128);
    }

    // OCB endgames: scale based on the number of remaining pieces of the strong side.
    else if (ocb_endgame(board))
        factor = 36 + popcount(board->color_bits[strong_side]) * 6;

    // Rook endgames: drawish if the pawn advantage is small, and all strong side pawns
    // are on the same side of the board. Don't scale if the defending king is far from
    // his own pawns.
    else if (strong_mat == ROOK_MG_SCORE && weak_mat == ROOK_MG_SCORE
        && (popcount(strong_pawns) - popcount(weak_pawns) < 2)
        && !!(KINGSIDE_BITS & strong_pawns) != !!(QUEENSIDE_BITS & strong_pawns)
        && (king_moves(board_king_square(board, weak_side)) & weak_pawns))
        factor = 64;

    // Naked weak king versus mating material: use a large value for scaling
    else if (!weak_mat && !weak_pawns && piecetypes_bb(board, ROOK, QUEEN))
        factor = 256;

    // Other endgames: scale based on the number of remaining pawns for the strong side.
    else
        factor = 128;

    eg = (score_t)((int32_t)eg * factor / 128);

    return (eg);
}
