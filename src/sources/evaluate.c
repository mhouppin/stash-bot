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
#include <string.h>
#include "endgame.h"
#include "engine.h"
#include "imath.h"
#include "pawns.h"

#ifdef TUNE
evaltrace_t Trace;
#endif

// Special eval terms

const scorepair_t CastlingBonus = SPAIR(49, -31);
const scorepair_t Initiative = SPAIR(17, 17);

// King Safety eval terms

const scorepair_t KnightWeight = SPAIR(25, 8);
const scorepair_t BishopWeight = SPAIR(18, 5);
const scorepair_t RookWeight = SPAIR(52, -4);
const scorepair_t QueenWeight = SPAIR(47, 72);

// Knight eval terms

const scorepair_t KnightShielded = SPAIR(5, 29);
const scorepair_t KnightOutpost = SPAIR(3, -20);
const scorepair_t KnightCenterOutpost = SPAIR(29, -3);
const scorepair_t KnightSolidOutpost = SPAIR(13, 42);

// Bishop eval terms

const scorepair_t BishopPairBonus = SPAIR(14, 112);
const scorepair_t BishopShielded = SPAIR(10, 28);

// Rook eval terms

const scorepair_t RookOnSemiOpenFile = SPAIR(15, 19);
const scorepair_t RookOnOpenFile = SPAIR(35, 15);
const scorepair_t RookXrayQueen = SPAIR(8, 12);

const scorepair_t MobilityN[9] = {
    SPAIR( -43, -44), SPAIR( -44, -33), SPAIR( -35,  19), SPAIR( -28,  50),
    SPAIR( -22,  63), SPAIR( -18,  79), SPAIR( -11,  80), SPAIR(  -3,  77),
    SPAIR(   8,  58)
};

const scorepair_t MobilityB[14] = {
    SPAIR( -64,-100), SPAIR( -39, -84), SPAIR( -34, -42), SPAIR( -25,  -6),
    SPAIR( -16,  16), SPAIR( -10,  38), SPAIR(  -8,  55), SPAIR( -11,  62),
    SPAIR( -10,  73), SPAIR(  -7,  73), SPAIR(  -1,  71), SPAIR(  15,  58),
    SPAIR(  27,  63), SPAIR(  74,  21)
};

const scorepair_t MobilityR[15] = {
    SPAIR( -56, -70), SPAIR( -51, -15), SPAIR( -42,  40), SPAIR( -43,  73),
    SPAIR( -40,  88), SPAIR( -40, 104), SPAIR( -37, 118), SPAIR( -33, 122),
    SPAIR( -30, 129), SPAIR( -24, 133), SPAIR( -21, 138), SPAIR( -19, 144),
    SPAIR( -17, 146), SPAIR(   1, 131), SPAIR(  57,  92)
};

const scorepair_t MobilityQ[28] = {
    SPAIR( -80,-186), SPAIR( -56,-112), SPAIR( -42, -23), SPAIR( -19,   8),
    SPAIR(  -6,  34), SPAIR(   0,  93), SPAIR(   5, 112), SPAIR(  10, 132),
    SPAIR(  14, 152), SPAIR(  19, 169), SPAIR(  24, 179), SPAIR(  28, 185),
    SPAIR(  31, 187), SPAIR(  33, 191), SPAIR(  35, 192), SPAIR(  34, 201),
    SPAIR(  31, 205), SPAIR(  26, 209), SPAIR(  26, 205), SPAIR(  27, 206),
    SPAIR(  26, 202), SPAIR(  31, 195), SPAIR(  25, 195), SPAIR(  30, 187),
    SPAIR(  43, 169), SPAIR(  16, 175), SPAIR(  37, 187), SPAIR(  55, 184)
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
    // Note that the KBNK case has already been handled at this point
    // in the eval, so it's not necessary to worry about it.

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
    // Check if there is exactly one White Bishop and one Black Bishop

    bitboard_t wbishop = piece_bb(board, WHITE, BISHOP);

    if (!wbishop || more_than_one(wbishop))
        return (false);

    bitboard_t bbishop = piece_bb(board, BLACK, BISHOP);

    if (!bbishop || more_than_one(bbishop))
        return (false);

    // Then check that the Bishops are on opposite colored squares

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

    // Be careful to cast to 32-bit integer here before multiplying to avoid overflows

    eg = (score_t)((int32_t)eg * factor / 128);
    TRACE_FACTOR(factor);

    return (eg);
}

void eval_init(const board_t *board, evaluation_t *eval)
{
    eval->attackers[WHITE] = eval->attackers[BLACK] = eval->weights[WHITE] = eval->weights[BLACK] = 0;

    square_t wksq = get_king_square(board, WHITE);
    square_t bksq = get_king_square(board, BLACK);

    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(wksq), WHITE, 1);
    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(bksq ^ SQ_A8), BLACK, 1);

    // Set the King Attack zone as the 3x4 square surrounding the king
    // (counting an additional rank in front of the king)

    eval->kingZone[WHITE] = king_moves(bksq);
    eval->kingZone[BLACK] = king_moves(wksq);
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

        TRACE_ADD(IDX_PIECE + KNIGHT - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (KNIGHT - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Knight is pinned, it has no Mobility squares

        if (board->stack->kingBlockers[us] & sqbb)
            b = 0;

        // Bonus for Knight mobility

        ret += MobilityN[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_KNIGHT + popcount(b & eval->mobilityZone[us]), us, 1);

        // Bonus for Knight with a pawn above it

        if (relative_shift_up(sqbb, us) & ourPawns)
        {
            ret += KnightShielded;
            TRACE_ADD(IDX_KNIGHT_SHIELDED, us, 1);
        }

        // Bonus for Knight on Outpost, with higher scores if the Knight is on
        // a center file, on the 6th rank, or supported by a pawn.

        if (sqbb & outpost & ~pe->attackSpan[not_color(us)])
        {
            ret += KnightOutpost;
            TRACE_ADD(IDX_KNIGHT_OUTPOST, us, 1);

            if (pawn_moves(sq, not_color(us)) & ourPawns)
            {
                ret += KnightSolidOutpost;
                TRACE_ADD(IDX_KNIGHT_SOLID_OUTPOST, us, 1);
            }

            if (sqbb & CENTER_FILES_BITS)
            {
                ret += KnightCenterOutpost;
                TRACE_ADD(IDX_KNIGHT_CENTER_OUTPOST, us, 1);
            }
        }

        // Bonus for a Knight on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * KnightWeight;
            TRACE_ADD(IDX_KS_KNIGHT, us, popcount(b & eval->kingZone[us]));
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
    {
        ret += BishopPairBonus;
        TRACE_ADD(IDX_BISHOP_PAIR, us, 1);
    }

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancy);

        TRACE_ADD(IDX_PIECE + BISHOP - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (BISHOP - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Bishop is pinned, reduce its mobility to all the squares
        // between the King and the pinner

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for Bishop mobility

        ret += MobilityB[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_BISHOP + popcount(b & eval->mobilityZone[us]), us, 1);

		// Bonus for Bishop with a pawn above it

        if (relative_shift_up(square_bb(sq), us) & ourPawns)
        {
            ret += BishopShielded;
            TRACE_ADD(IDX_BISHOP_SHIELDED, us, 1);
        }

        // Bonus for a Bishop on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * BishopWeight;
            TRACE_ADD(IDX_KS_BISHOP, us, popcount(b & eval->kingZone[us]));
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

        TRACE_ADD(IDX_PIECE + ROOK - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (ROOK - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Rook is pinned, reduce its mobility to all the squares
        // between the King and the pinner

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for a Rook on an open (or semi-open) file

        if (!(rookFile & ourPawns))
        {
            ret += (rookFile & theirPawns) ? RookOnSemiOpenFile : RookOnOpenFile;
            TRACE_ADD((rookFile & theirPawns) ? IDX_ROOK_SEMIOPEN : IDX_ROOK_OPEN, us, 1);
        }

        // Bonus for a Rook on the same file as the opponent's Queen(s)

        if (rookFile & theirQueens)
        {
            ret += RookXrayQueen;
            TRACE_ADD(IDX_ROOK_XRAY_QUEEN, us, 1);
        }

        // Bonus for Rook mobility

        ret += MobilityR[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_ROOK + popcount(b & eval->mobilityZone[us]), us, 1);

        // Bonus for a Rook on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * RookWeight;
            TRACE_ADD(IDX_KS_ROOK, us, popcount(b & eval->kingZone[us]));
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

        TRACE_ADD(IDX_PIECE + QUEEN - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (QUEEN - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Queen is pinned, reduce its mobility to all the squares
        // between the King and the pinner

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Bonus for Queen mobility

        ret += MobilityQ[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_QUEEN + popcount(b & eval->mobilityZone[us]), us, 1);

        // Bonus for a Queen on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->attackers[us] += 1;
            eval->weights[us] += popcount(b & eval->kingZone[us]) * QueenWeight;
            TRACE_ADD(IDX_KS_QUEEN, us, popcount(b & eval->kingZone[us]));
        }
    }
    return (ret);
}

scorepair_t evaluate_safety(evaluation_t *eval, color_t us)
{
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone

    TRACE_ATTACKERS(us, eval->attackers[us]);

    if (eval->attackers[us] >= 2)
    {
        scorepair_t bonus = eval->weights[us];

        // Reduce the King Safety bonus as the number of attackers on the King
        // gets lower

        if (eval->attackers[us] < 8)
            bonus -= scorepair_divide(bonus, AttackRescale[eval->attackers[us]]);

        return (bonus);
    }
    return (0);
}

score_t evaluate(const board_t *board)
{
    TRACE_INIT;

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
    {
        tapered += CastlingBonus;
        TRACE_ADD(IDX_CASTLING, WHITE, 1);
    }
    if (board->stack->castlings & BLACK_CASTLING)
    {
        tapered -= CastlingBonus;
        TRACE_ADD(IDX_CASTLING, BLACK, 1);
    }

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

    int tempoValue = (eval.tempos[WHITE] * eval.tempos[WHITE] - eval.tempos[BLACK] * eval.tempos[BLACK]);
    tapered += Initiative * tempoValue;
    TRACE_ADD(IDX_INITIATIVE, WHITE, tempoValue);

    TRACE_EVAL(tapered);

    mg = midgame_score(tapered);

    // Scale endgame score based on remaining material + pawns
    eg = scale_endgame(board, endgame_score(tapered));

    // Compute the eval by interpolating between the middlegame and endgame scores

    {
        int phase = 4 * popcount(piecetype_bb(board, QUEEN))
            + 2 * popcount(piecetype_bb(board, ROOK))
            + popcount(piecetypes_bb(board, KNIGHT, BISHOP));

        if (phase >= 24)
        {
            score = mg;
            TRACE_PHASE(24);
        }
        else
        {
            score = mg * phase / 24;
            score += eg * (24 - phase) / 24;
            TRACE_PHASE(phase);
        }
    }

    // Return the score relative to the side to move

    return (board->sideToMove == WHITE ? score : -score);
}
