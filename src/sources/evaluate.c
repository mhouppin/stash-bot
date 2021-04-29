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
#include "endgame.h"
#include "engine.h"
#include "imath.h"
#include "pawns.h"

enum
{
    // Special eval terms

    CastlingBonus = SPAIR(85, -43),
    Initiative = SPAIR(21, 15),

    // King Safety eval terms

    KnightWeight = SPAIR(26, 8),
    BishopWeight = SPAIR(18, 5),
    RookWeight = SPAIR(51, -4),
    QueenWeight = SPAIR(51, 72),

	// Knight eval terms

    KnightShielded = SPAIR(4, 12),
    KnightOutpost = SPAIR(15, -3),
    KnightCenterOutpost = SPAIR(17, -1),
    KnightSolidOutpost = SPAIR(12, 1),

    // Bishop eval terms

    BishopPairBonus = SPAIR(12, 103),
    BishopShielded = SPAIR(12, 6),

    // Rook eval terms

    RookOnSemiOpenFile = SPAIR(19, 17),
    RookOnOpenFile = SPAIR(38, 16),
    RookXrayQueen = SPAIR(7, 9),

    QueenPhase = 4,
    RookPhase = 2,
    MinorPhase = 1,

    MidgamePhase = 24,
};

const scorepair_t MobilityN[9] = {
    SPAIR( -83, -76), SPAIR( -40, -74), SPAIR( -24, -15), SPAIR( -16,  26),
    SPAIR(  -4,  32), SPAIR(  -3,  49), SPAIR(   4,  53), SPAIR(  13,  48),
    SPAIR(  26,  31)
};

const scorepair_t MobilityB[14] = {
    SPAIR( -96, -80), SPAIR( -51,-100), SPAIR( -16, -66), SPAIR( -15, -24),
    SPAIR(  -5,  -1), SPAIR(   1,  16), SPAIR(   4,  33), SPAIR(   4,  41),
    SPAIR(   4,  49), SPAIR(   5,  54), SPAIR(   8,  52), SPAIR(  19,  44),
    SPAIR(  44,  43), SPAIR(  46,  26)
};

const scorepair_t MobilityR[15] = {
    SPAIR( -43, -11), SPAIR( -56, -14), SPAIR( -37,  -6), SPAIR( -35,  22),
    SPAIR( -34,  59), SPAIR( -33,  69), SPAIR( -31,  84), SPAIR( -28,  91),
    SPAIR( -24,  94), SPAIR( -19,  98), SPAIR( -15, 104), SPAIR( -13, 105),
    SPAIR(  -8, 105), SPAIR(   7,  95), SPAIR(  54,  64)
};

const scorepair_t MobilityQ[28] = {
    SPAIR(  -8,-144), SPAIR(  -6,-117), SPAIR(  -5, -90), SPAIR(  -7, -63),
    SPAIR( -13, -38), SPAIR(  -6, -11), SPAIR(   5,  26), SPAIR(   7,  74),
    SPAIR(  12, 101), SPAIR(  15, 125), SPAIR(  19, 137), SPAIR(  21, 159),
    SPAIR(  25, 170), SPAIR(  30, 173), SPAIR(  30, 183), SPAIR(  30, 189),
    SPAIR(  29, 191), SPAIR(  25, 197), SPAIR(  24, 198), SPAIR(  21, 196),
    SPAIR(  33, 186), SPAIR(  33, 183), SPAIR(  31, 171), SPAIR(  31, 167),
    SPAIR(  21, 156), SPAIR(  15, 151), SPAIR(  15, 145), SPAIR(  17, 146)
};

const int AttackRescale[8] = {
    0, 0, 2, 4, 8, 16, 32, 64
};

typedef struct evaluation_s
{
    bitboard_t kingZone[COLOR_NB];
    bitboard_t mobilityZone[COLOR_NB];
    int attackers[COLOR_NB];
    scorepair_t weights[COLOR_NB];
    int tempos[COLOR_NB];
}
evaluation_t;

bool is_kxk_endgame(const board_t *board, color_t us)
{
    // Weak side has pieces or pawns, this is not a KXK endgame

    if (more_than_one(color_bb(board, not_color(us))))
        return (false);

    return (board->stack->material[us] >= ROOK_MG_SCORE);
}

score_t eval_kxk(const board_t *board, color_t us)
{
    // Be careful to avoid stalemating the weak king
    if (board->sideToMove != us && !board->stack->checkers)
    {
        movelist_t list;

        list_all(&list, board);
        if (movelist_size(&list) == 0)
            return (0);
    }

    square_t winningKsq = get_king_square(board, us);
    square_t losingKsq = get_king_square(board, not_color(us));
    score_t score = board->stack->material[us] + popcount(piecetype_bb(board, PAWN)) * PAWN_MG_SCORE;

    // Push the weak king to the corner

    score += edge_bonus(losingKsq);

    // Give a bonus for close kings

    score += close_bonus(winningKsq, losingKsq);

    // Set the score as winning if we have mating material:
    // - a major piece;
    // - a bishop and a knight;
    // - two opposite colored bishops;
    // - three knights.

    bitboard_t knights = piecetype_bb(board, KNIGHT);
    bitboard_t bishops = piecetype_bb(board, BISHOP);

    if (piecetype_bb(board, QUEEN) || piecetype_bb(board, ROOK)
        || (knights && bishops)
        || ((bishops & DARK_SQUARES) && (bishops & ~DARK_SQUARES))
        || (popcount(knights) >= 3))
        score += VICTORY;

    return (board->sideToMove == us ? score : -score);
}

bool ocb_endgame(const board_t *board)
{
    bitboard_t wbishop = piece_bb(board, WHITE, BISHOP);

    if (!wbishop || more_than_one(wbishop))
        return (false);

    bitboard_t bbishop = piece_bb(board, BLACK, BISHOP);

    if (!bbishop || more_than_one(bbishop))
        return (false);

    bitboard_t dsqMask = (wbishop | bbishop) & DARK_SQUARES;

    return (!!dsqMask && !more_than_one(dsqMask));
}

score_t scale_endgame(const board_t *board, score_t eg)
{
    // Only detect scalable endgames from the side with a positive evaluation.
    // This allows us to quickly filter out positions which shouldn't be scaled,
    // even though they have a theoretical scaling factor in our code (like KNvKPPPP).

    color_t strongSide = (eg > 0) ? WHITE : BLACK, weakSide = not_color(strongSide);
    int factor;
    score_t strongMat = board->stack->material[strongSide], weakMat = board->stack->material[weakSide];
    bitboard_t strongPawns = piece_bb(board, strongSide, PAWN), weakPawns = piece_bb(board, weakSide, PAWN);

    // No pawns and low material difference, the endgame is either drawn
    // or very difficult to win.
    if (!strongPawns && strongMat - weakMat <= BISHOP_MG_SCORE)
        factor = (strongMat <= BISHOP_MG_SCORE) ? 0 : clamp((strongMat - weakMat) / 8, 8, 32);

    // OCB endgames: scale based on the number of remaining pieces of the strong side.
    else if (ocb_endgame(board))
        factor = 36 + popcount(color_bb(board, strongSide)) * 6;

    // Rook endgames: drawish if the pawn advantage is small, and all strong side pawns
    // are on the same side of the board. Don't scale if the defending king is far from
    // his own pawns.
    else if (strongMat == ROOK_MG_SCORE && weakMat == ROOK_MG_SCORE
        && (popcount(strongPawns) - popcount(weakPawns) < 2)
        && !!(KINGSIDE_BITS & strongPawns) != !!(QUEENSIDE_BITS & strongPawns)
        && (king_moves(get_king_square(board, weakSide)) & weakPawns))
        factor = 64;

    // Other endgames. Decrease the endgame score as the number of pawns of the strong
    // side gets lower.
    else
        factor = min(128, 96 + 8 * popcount(strongPawns));

    eg = (score_t)((int32_t)eg * factor / 128);

    return (eg);
}

void        eval_init(const board_t *board, evaluation_t *eval)
{
    eval->attackers[WHITE] = eval->attackers[BLACK] = eval->weights[WHITE] = eval->weights[BLACK] = 0;

    // Set the King Attack zone as the 3x4 square surrounding the king
    // (counting an additional rank in front of the king)

    eval->kingZone[WHITE] = king_moves(get_king_square(board, BLACK));
    eval->kingZone[BLACK] = king_moves(get_king_square(board, WHITE));
    eval->kingZone[WHITE] |= shift_down(eval->kingZone[WHITE]);
    eval->kingZone[BLACK] |= shift_up(eval->kingZone[BLACK]);

    bitboard_t occupied = occupancy_bb(board);
    bitboard_t wpawns = piece_bb(board, WHITE, PAWN);
    bitboard_t bpawns = piece_bb(board, BLACK, PAWN);
    bitboard_t wattacks = wpawns_attacks_bb(wpawns);
    bitboard_t battacks = bpawns_attacks_bb(bpawns);

    // Exclude opponent pawns' attacks from the Mobility and King Attack zones

    eval->mobilityZone[WHITE] = ~battacks;
    eval->mobilityZone[BLACK] = ~wattacks;
    eval->kingZone[WHITE] &= eval->mobilityZone[WHITE];
    eval->kingZone[BLACK] &= eval->mobilityZone[BLACK];

    // Exclude rammed pawns and our pawns on rank 2 and 3 from mobility zone

    eval->mobilityZone[WHITE] &= ~(wpawns & (shift_down(occupied) | RANK_2_BITS | RANK_3_BITS));
    eval->mobilityZone[BLACK] &= ~(bpawns & (shift_up(occupied) | RANK_6_BITS | RANK_7_BITS));

    // Add pawn attacks on opponent's pieces as tempos

    eval->tempos[WHITE] = popcount(color_bb(board, BLACK) & ~piecetype_bb(board, PAWN) & wattacks);
    eval->tempos[BLACK] = popcount(color_bb(board, WHITE) & ~piecetype_bb(board, PAWN) & battacks);

    // If not in check, add one tempo to the side to move

    eval->tempos[board->sideToMove] += !board->stack->checkers;
}

scorepair_t evaluate_knights(const board_t *board, evaluation_t *eval, const pawn_entry_t *pe, color_t us)
{
    scorepair_t ret = 0;
    bitboard_t bb = piece_bb(board, us, KNIGHT);
    bitboard_t targets = pieces_bb(board, not_color(us), ROOK, QUEEN);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);
    bitboard_t outpost = RANK_4_BITS | RANK_5_BITS | (us == WHITE ? RANK_6_BITS : RANK_3_BITS);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = knight_moves(sq);

        if (board->stack->kingBlockers[us] & sqbb)
            b = 0;

        // Bonus for Knight mobility

        ret += MobilityN[popcount(b & eval->mobilityZone[us])];

        // Bonus for Knight with a pawn above it

        if (relative_shift_up(sqbb, us) & ourPawns)
            ret += KnightShielded;

        // Bonus for Knight on Outpost, with higher scores if the Knight is on
        // a center file, on the 6th rank, or supported by a pawn.

        if (sqbb & outpost & ~pe->attackSpan[not_color(us)])
        {
            ret += KnightOutpost;

            if (pawn_moves(sq, not_color(us)) & ourPawns)
                ret += KnightSolidOutpost;

            if (sqbb & CENTER_FILES_BITS)
                ret += KnightCenterOutpost;
        }

        // Bonus for a Knight on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * KnightWeight;
        }

        // Tempo bonus for a Knight attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[us] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_bishops(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    bitboard_t bb = piece_bb(board, us, BISHOP);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);
    bitboard_t targets = pieces_bb(board, not_color(us), ROOK, QUEEN);

    // Bonus for the Bishop pair

    if (more_than_one(bb))
        ret += BishopPairBonus;

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancy);

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for Bishop mobility

        ret += MobilityB[popcount(b & eval->mobilityZone[us])];

		// Bonus for Bishop with a pawn above it

        if (relative_shift_up(square_bb(sq), us) & ourPawns)
            ret += BishopShielded;

        // Bonus for a Bishop on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * BishopWeight;
        }

        // Tempo bonus for a Bishop attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[us] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_rooks(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    const bitboard_t ourPawns = piece_bb(board, us, PAWN);
    const bitboard_t theirPawns = piece_bb(board, not_color(us), PAWN);
    const bitboard_t theirQueens = piece_bb(board, not_color(us), QUEEN);
    bitboard_t bb = piece_bb(board, us, ROOK);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t rookFile = sq_file_bb(sq);
        bitboard_t b = rook_moves_bb(sq, occupancy);

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for a Rook on an open (or semi-open) file

        if (!(rookFile & ourPawns))
            ret += (rookFile & theirPawns) ? RookOnSemiOpenFile : RookOnOpenFile;

        // Bonus for a Rook on the same file as the opponent's Queen(s)

        if (rookFile & theirQueens)
            ret += RookXrayQueen;

        // Bonus for Rook mobility

        ret += MobilityR[popcount(b & eval->mobilityZone[us])];

        // Bonus for a Rook on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * RookWeight;
        }

        // Tempo bonus for a Rook attacking the opponent's Queen(s)

        if (b & theirQueens)
            eval->tempos[us] += popcount(b & theirQueens);
    }
    return (ret);
}

scorepair_t evaluate_queens(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    bitboard_t bb = piece_bb(board, us, QUEEN);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancy) | rook_moves_bb(sq, occupancy);

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for Queen mobility

        ret += MobilityQ[popcount(b & eval->mobilityZone[us])];

        // Bonus for a Queen on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * QueenWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_safety(evaluation_t *eval, color_t us)
{
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone

    if (eval->attackers[us] >= 2)
    {
        scorepair_t bonus = eval->weights[us];

        if (eval->attackers[us] < 8)
            bonus -= scorepair_divide(bonus, AttackRescale[eval->attackers[us]]);

        return (bonus);
    }
    return (0);
}

score_t evaluate(const board_t *board)
{
    // Do we have a specialized endgame eval for the current configuration ?
    const endgame_entry_t *entry = endgame_probe(board);

    if (entry != NULL)
        return (entry->func(board, entry->winningSide));

    // Is there a KXK situation ? (lone King vs mating material)

    if (is_kxk_endgame(board, WHITE))
        return (eval_kxk(board, WHITE));
    if (is_kxk_endgame(board, BLACK))
        return (eval_kxk(board, BLACK));

    evaluation_t eval;
    scorepair_t tapered = board->psqScorePair;
    pawn_entry_t *pe;
    score_t mg, eg, score;

    // Bonus for having castling rights in the middlegame
    // (castled King bonus values more than castling right bonus to incite
    // castling in the opening and quick attacks on an uncastled King)

    if (board->stack->castlings & WHITE_CASTLING)
        tapered += CastlingBonus;
    if (board->stack->castlings & BLACK_CASTLING)
        tapered -= CastlingBonus;

    eval_init(board, &eval);

    // Add the pawn structure evaluation

    pe = pawn_probe(board);
    tapered += pe->value;

    // Add the pieces' evaluation

    tapered += evaluate_knights(board, &eval, pe, WHITE);
    tapered -= evaluate_knights(board, &eval, pe, BLACK);
    tapered += evaluate_bishops(board, &eval, WHITE);
    tapered -= evaluate_bishops(board, &eval, BLACK);
    tapered += evaluate_rooks(board, &eval, WHITE);
    tapered -= evaluate_rooks(board, &eval, BLACK);
    tapered += evaluate_queens(board, &eval, WHITE);
    tapered -= evaluate_queens(board, &eval, BLACK);

    // Add the King Safety evaluation

    tapered += evaluate_safety(&eval, WHITE);
    tapered -= evaluate_safety(&eval, BLACK);

    // Compute Initiative based on how many tempos each side have. The scaling
    // is quadratic so that hanging pieces that can be captured are easily spotted
    // by the eval

    tapered += Initiative * (eval.tempos[WHITE] * eval.tempos[WHITE] - eval.tempos[BLACK] * eval.tempos[BLACK]);

    mg = midgame_score(tapered);

    // Scale endgame score based on remaining material + pawns
    eg = scale_endgame(board, endgame_score(tapered));

    // Compute the eval by interpolating between the middlegame and endgame scores

    {
        int phase = QueenPhase * popcount(piecetype_bb(board, QUEEN))
            + RookPhase * popcount(piecetype_bb(board, ROOK))
            + MinorPhase * popcount(piecetypes_bb(board, KNIGHT, BISHOP));

        if (phase >= MidgamePhase)
            score = mg;
        else
        {
            score = mg * phase / MidgamePhase;
            score += eg * (MidgamePhase - phase) / MidgamePhase;
        }
    }

    // Return the score relative to the side to move

    return (board->sideToMove == WHITE ? score : -score);
}
