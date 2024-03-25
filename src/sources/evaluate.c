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

#include "evaluate.h"
#include "endgame.h"
#include "movelist.h"
#include "pawns.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>

#ifdef TUNE
evaltrace_t Trace;
#endif

// clang-format off

// Special eval terms
const scorepair_t CastlingBonus = SPAIR(98, -48);
const scorepair_t Initiative = SPAIR(22, 19);

// King Safety eval terms
const scorepair_t KnightWeight    = SPAIR(  48,  17);
const scorepair_t BishopWeight    = SPAIR(  37,  30);
const scorepair_t RookWeight      = SPAIR(  39, -35);
const scorepair_t QueenWeight     = SPAIR(   9,   7);
const scorepair_t AttackWeight    = SPAIR(   7,   8);
const scorepair_t WeakKingZone    = SPAIR(  27, -23);
const scorepair_t SafeKnightCheck = SPAIR(  74,   8);
const scorepair_t SafeBishopCheck = SPAIR(  36,  31);
const scorepair_t SafeRookCheck   = SPAIR(  94,  42);
const scorepair_t SafeQueenCheck  = SPAIR(  48,  53);
const scorepair_t UnsafeCheck     = SPAIR(  20,  35);
const scorepair_t QueenlessAttack = SPAIR( -78,  -7);
const scorepair_t SafetyOffset    = SPAIR(  21,  34);

// Storm/Shelter indexes:
// 0-7 - Side
// 9-15 - Front
// 16-23 - Center
const scorepair_t KingStorm[24] = {
    SPAIR(  -9,  -2), SPAIR( -12,  -1), SPAIR(  17,   4), SPAIR( -11,   4),
    SPAIR( -28,  11), SPAIR( -20,   2), SPAIR( -46,   3), SPAIR( -16,   0),
    SPAIR(   0,   0), SPAIR( -10,  -5), SPAIR(  41,  10), SPAIR(   3,  -0),
    SPAIR(  -6,   1), SPAIR(  -7,  10), SPAIR(   6,  18), SPAIR(  14,  -1),
    SPAIR(  -1,  -1), SPAIR(  -2,  -0), SPAIR(  36,   3), SPAIR(  25,  -3),
    SPAIR(  -7,   1), SPAIR(  -8,  16), SPAIR(  -3,  35), SPAIR(   2, -14)
};

const scorepair_t KingShelter[24] = {
    SPAIR( -45,   7), SPAIR( -34,  42), SPAIR( -34, -11), SPAIR( -24,   6),
    SPAIR(   9,  -1), SPAIR(   6,  -0), SPAIR(   6,  -0), SPAIR( -11, -22),
    SPAIR(   0,   0), SPAIR(  -5,  -1), SPAIR(  -2,  32), SPAIR(  -2,  15),
    SPAIR(  10,   6), SPAIR(  21,   0), SPAIR(   7,   0), SPAIR(  16, -19),
    SPAIR( -41, -10), SPAIR(   9, -37), SPAIR(   3,  23), SPAIR(  15,  39),
    SPAIR(  11,   8), SPAIR(  23,   0), SPAIR(   6,  -0), SPAIR(  18,  11)
};

// Knight eval terms
const scorepair_t KnightShielded      = SPAIR(  4, 11);
const scorepair_t KnightOutpost       = SPAIR( 15,-18);
const scorepair_t KnightCenterOutpost = SPAIR(  9, 13);
const scorepair_t KnightSolidOutpost  = SPAIR( 22, 21);

const scorepair_t ClosedPosKnight[5] = {
    SPAIR(  10, -22), SPAIR(   9,   1), SPAIR(   8,  20), SPAIR(  11,  33),
    SPAIR(  12,  44)
};

// Bishop eval terms
const scorepair_t BishopPairBonus    = SPAIR( 24, 98);
const scorepair_t BishopShielded     = SPAIR(  3,  5);
const scorepair_t BishopLongDiagonal = SPAIR( 13, 41);

const scorepair_t BishopPawnsSameColor[7] = {
    SPAIR(  11,  38), SPAIR(  17,  31), SPAIR(  16,  16), SPAIR(  11,   9),
    SPAIR(   6,   0), SPAIR(   2,  -6), SPAIR(  -2, -15)
};

// Rook eval terms
const scorepair_t RookOnSemiOpenFile = SPAIR( 15, 29);
const scorepair_t RookOnOpenFile     = SPAIR( 32, 14);
const scorepair_t RookOnBlockedFile  = SPAIR( -4,-18);
const scorepair_t RookXrayQueen      = SPAIR( 15,  3);

// Mobility eval terms
const scorepair_t MobilityN[9] = {
    SPAIR( -62,  11), SPAIR( -43, -35), SPAIR( -32,  31), SPAIR( -23,  58),
    SPAIR( -16,  78), SPAIR( -12,  98), SPAIR(  -7, 108), SPAIR(  -1, 114),
    SPAIR(   3, 110)
};

const scorepair_t MobilityB[14] = {
    SPAIR( -57, -47), SPAIR( -42, -35), SPAIR( -25, -10), SPAIR( -25,  22),
    SPAIR( -17,  40), SPAIR(  -9,  50), SPAIR(  -4,  60), SPAIR(  -6,  63),
    SPAIR(  -7,  66), SPAIR(  -1,  61), SPAIR(  -0,  61), SPAIR(   9,  48),
    SPAIR(   7,  42), SPAIR(  13,  34)
};

const scorepair_t MobilityR[15] = {
    SPAIR(-117,  -7), SPAIR( -46,  19), SPAIR( -31,  67), SPAIR( -32,  89),
    SPAIR( -28, 111), SPAIR( -29, 125), SPAIR( -27, 131), SPAIR( -24, 134),
    SPAIR( -18, 145), SPAIR(  -7, 148), SPAIR(  -8, 155), SPAIR(  -5, 161),
    SPAIR(  11, 160), SPAIR(  18, 155), SPAIR(  27, 154)
};

const scorepair_t MobilityQ[28] = {
    SPAIR( -67,-155), SPAIR(  28, 165), SPAIR(   6, 159), SPAIR(   8, 136),
    SPAIR(   6,  88), SPAIR(   7, 121), SPAIR(   5, 149), SPAIR(   5, 176),
    SPAIR(   8, 195), SPAIR(  10, 206), SPAIR(   9, 215), SPAIR(   9, 224),
    SPAIR(  10, 228), SPAIR(  10, 232), SPAIR(  12, 233), SPAIR(   9, 236),
    SPAIR(  10, 239), SPAIR(  13, 234), SPAIR(  17, 224), SPAIR(  31, 215),
    SPAIR(  37, 208), SPAIR(  39, 197), SPAIR(  35, 191), SPAIR(  27, 177),
    SPAIR(  43, 154), SPAIR(  -7, 167), SPAIR(  14, 178), SPAIR(  46, 157)
};

// Threat eval terms
const scorepair_t PawnAttacksMinor  = SPAIR( 66, 87);
const scorepair_t PawnAttacksRook   = SPAIR( 63, 60);
const scorepair_t PawnAttacksQueen  = SPAIR( 55, 11);
const scorepair_t MinorAttacksRook  = SPAIR( 66, 54);
const scorepair_t MinorAttacksQueen = SPAIR( 50, 56);
const scorepair_t RookAttacksQueen  = SPAIR( 61, 42);

// clang-format on

typedef struct evaluation_s
{
    bitboard_t kingZone[COLOR_NB];
    bitboard_t mobilityZone[COLOR_NB];
    bitboard_t attacked[COLOR_NB];
    bitboard_t attackedTwice[COLOR_NB];
    bitboard_t attackedBy[COLOR_NB][PIECETYPE_NB];
    int safetyAttackers[COLOR_NB];
    int safetyAttacks[COLOR_NB];
    scorepair_t safetyScore[COLOR_NB];
    int positionClosed;
} evaluation_t;

bool is_kxk_endgame(const Board *board, color_t us)
{
    // Weak side has pieces or Pawns, this is not a KXK endgame.
    if (more_than_one(color_bb(board, not_color(us)))) return false;

    return board->stack->material[us] >= ROOK_MG_SCORE;
}

score_t eval_kxk(const Board *board, color_t us)
{
    // Be careful to avoid stalemating the weak King.
    if (board->sideToMove != us && !board->stack->checkers)
    {
        Movelist list;

        list_all(&list, board);
        if (movelist_size(&list) == 0) return 0;
    }

    square_t winningKsq = get_king_square(board, us);
    square_t losingKsq = get_king_square(board, not_color(us));
    score_t score =
        board->stack->material[us] + popcount(piecetype_bb(board, PAWN)) * PAWN_MG_SCORE;

    // Push the weak King to the corner.
    score += edge_bonus(losingKsq);

    // Give a bonus for close Kings.
    score += close_bonus(winningKsq, losingKsq);

    // Set the score as winning if we have mating material:
    // - a major piece;
    // - a Bishop and a Knight;
    // - two opposite colored Bishops;
    // - three Knights.
    // Note that the KBNK case has already been handled at this point
    // in the eval, so it's not necessary to worry about it.
    bitboard_t knights = piecetype_bb(board, KNIGHT);
    bitboard_t bishops = piecetype_bb(board, BISHOP);

    if (piecetype_bb(board, QUEEN) || piecetype_bb(board, ROOK) || (knights && bishops)
        || ((bishops & DSQ_BB) && (bishops & LSQ_BB)) || (popcount(knights) >= 3))
        score += VICTORY;

    return board->sideToMove == us ? score : -score;
}

bool ocb_endgame(const Board *board)
{
    // Check if there is exactly one White Bishop and one Black Bishop.
    bitboard_t wbishop = piece_bb(board, WHITE, BISHOP);

    if (!wbishop || more_than_one(wbishop)) return false;

    bitboard_t bbishop = piece_bb(board, BLACK, BISHOP);

    if (!bbishop || more_than_one(bbishop)) return false;

    // Then check that the Bishops are on opposite colored squares.
    bitboard_t dsqMask = (wbishop | bbishop) & DSQ_BB;

    return !!dsqMask && !more_than_one(dsqMask);
}

score_t scale_endgame(const Board *board, const KingPawnEntry *kpe, score_t eg)
{
    // Only detect scalable endgames from the side with a positive evaluation.
    // This allows us to quickly filter out positions which shouldn't be scaled,
    // even though they have a theoretical scaling factor in our code (like KNvKPPPP).
    color_t strongSide = (eg > 0) ? WHITE : BLACK, weakSide = not_color(strongSide);
    const EndgameEntry *entry;
    int factor;
    score_t strongMat = board->stack->material[strongSide],
            weakMat = board->stack->material[weakSide];
    bitboard_t strongPawns = piece_bb(board, strongSide, PAWN),
               weakPawns = piece_bb(board, weakSide, PAWN);

    // No Pawns and low material difference, the endgame is either drawn
    // or very difficult to win.
    if (!strongPawns && strongMat - weakMat <= BISHOP_MG_SCORE)
        factor = (strongMat <= BISHOP_MG_SCORE)
                     ? SCALE_DRAW
                     : imax((int32_t)(strongMat - weakMat) * 8 / BISHOP_MG_SCORE, 0);

    // OCB endgames: scale based on the number of remaining pieces of the strong side,
    // or if there are no other remaining pieces, based on the number of passed pawns.
    else if (ocb_endgame(board))
        factor = (strongMat + weakMat > 2 * BISHOP_MG_SCORE)
                     ? 71 + popcount(color_bb(board, strongSide)) * 9
                     : 33 + popcount(kpe->passed[strongSide]) * 21;

    // Rook endgames: drawish if the Pawn advantage is small, and all strong side Pawns
    // are on the same side of the board. Don't scale if the defending King is far from
    // his own Pawns.
    else if (strongMat == ROOK_MG_SCORE && weakMat == ROOK_MG_SCORE
             && (popcount(strongPawns) - popcount(weakPawns) < 2)
             && !!(KINGSIDE_BB & strongPawns) != !!(QUEENSIDE_BB & strongPawns)
             && (king_moves(get_king_square(board, weakSide)) & weakPawns))
        factor = 130;

    // Check if we have a specialized function for the given material distribution.
    else if ((entry = endgame_probe_scalefactor(board)) != NULL)
        factor = entry->scaleFunc(board, strongSide);

    // Other endgames. Decrease the endgame score as the number of pawns of the strong
    // side gets lower.
    else
        factor = imin(SCALE_NORMAL, 177 + 13 * popcount(strongPawns));

    // Be careful to cast to 32-bit integer here before multiplying to avoid overflows.
    eg = (score_t)((int32_t)eg * factor / SCALE_NORMAL);
    TRACE_FACTOR(factor);

    return eg;
}

void eval_init(const Board *board, const KingPawnEntry *kpe, evaluation_t *eval)
{
    memset(eval, 0, sizeof(evaluation_t));

    square_t wksq = get_king_square(board, WHITE);
    square_t bksq = get_king_square(board, BLACK);

    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(wksq), WHITE, 1);
    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(bksq ^ SQ_A8), BLACK, 1);

    // Set the King Attack zone as the 3x4 square surrounding the King
    // (counting an additional rank in front of the King). Initialize the attack
    // tables at the same time.
    eval->kingZone[WHITE] = king_moves(bksq);
    eval->kingZone[BLACK] = king_moves(wksq);

    eval->attacked[WHITE] = eval->attackedBy[WHITE][KING] = eval->kingZone[BLACK];
    eval->attacked[BLACK] = eval->attackedBy[BLACK][KING] = eval->kingZone[WHITE];

    eval->kingZone[WHITE] |= shift_down(eval->kingZone[WHITE]);
    eval->kingZone[BLACK] |= shift_up(eval->kingZone[BLACK]);

    bitboard_t occupied = occupancy_bb(board);
    bitboard_t wpawns = piece_bb(board, WHITE, PAWN);
    bitboard_t bpawns = piece_bb(board, BLACK, PAWN);
    bitboard_t wattacks = kpe->attacks[WHITE];
    bitboard_t battacks = kpe->attacks[BLACK];

    eval->attackedBy[WHITE][PAWN] = wattacks;
    eval->attackedBy[BLACK][PAWN] = battacks;
    eval->attackedTwice[WHITE] |= eval->attacked[WHITE] & wattacks;
    eval->attackedTwice[BLACK] |= eval->attacked[BLACK] & wattacks;
    eval->attackedTwice[WHITE] |= kpe->attacks2[WHITE];
    eval->attackedTwice[BLACK] |= kpe->attacks2[BLACK];
    eval->attacked[WHITE] |= wattacks;
    eval->attacked[BLACK] |= battacks;

    // Exclude opponent Pawns' attacks from the Mobility and King Attack zones.
    eval->mobilityZone[WHITE] = ~battacks;
    eval->mobilityZone[BLACK] = ~wattacks;
    eval->kingZone[WHITE] &= eval->mobilityZone[WHITE];
    eval->kingZone[BLACK] &= eval->mobilityZone[BLACK];

    // Exclude rammed Pawns and our Pawns on ranks 2 and 3 from the Mobility zone.
    eval->mobilityZone[WHITE] &= ~(wpawns & (shift_down(occupied) | RANK_2_BB | RANK_3_BB));
    eval->mobilityZone[BLACK] &= ~(bpawns & (shift_up(occupied) | RANK_6_BB | RANK_7_BB));

    // Exclude our King from the Mobility zone.
    eval->mobilityZone[WHITE] &= ~square_bb(wksq);
    eval->mobilityZone[BLACK] &= ~square_bb(bksq);

    // Compute position closedness based on the number of Pawns who cannot advance
    // (including Pawns with their stop squares controlled).
    bitboard_t fixedPawns = wpawns & shift_down(occupied | battacks);
    fixedPawns |= bpawns & shift_up(occupied | wattacks);

    eval->positionClosed = imin(4, popcount(fixedPawns) / 2);
}

scorepair_t evaluate_knights(
    const Board *board, evaluation_t *eval, const KingPawnEntry *kpe, color_t us)
{
    scorepair_t ret = 0;
    bitboard_t bb = piece_bb(board, us, KNIGHT);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);
    bitboard_t outpost = RANK_4_BB | RANK_5_BB | (us == WHITE ? RANK_6_BB : RANK_3_BB);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = knight_moves(sq);

        TRACE_ADD(IDX_PIECE + KNIGHT - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (KNIGHT - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // Give a bonus/penalty based on how closed the position is.
        ret += ClosedPosKnight[eval->positionClosed];
        TRACE_ADD(IDX_KNIGHT_CLOSED_POS + eval->positionClosed, us, 1);

        // If the Knight is pinned, it has no Mobility squares.
        if (board->stack->kingBlockers[us] & sqbb) b = 0;

        // Update the attack tables.
        eval->attackedBy[us][KNIGHT] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Give a bonus for the Knight's mobility.
        ret += MobilityN[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_KNIGHT + popcount(b & eval->mobilityZone[us]), us, 1);

        // give a bonus for a Knight with a Pawn above it.
        if (relative_shift_up(sqbb, us) & ourPawns)
        {
            ret += KnightShielded;
            TRACE_ADD(IDX_KNIGHT_SHIELDED, us, 1);
        }

        // Give a bonus for a Knight on an Outpost, with higher scores if it is
        // on a center file, or supported by a Pawn.
        if (sqbb & outpost & ~kpe->attackSpan[not_color(us)])
        {
            ret += KnightOutpost;
            TRACE_ADD(IDX_KNIGHT_OUTPOST, us, 1);

            if (pawn_moves(sq, not_color(us)) & ourPawns)
            {
                ret += KnightSolidOutpost;
                TRACE_ADD(IDX_KNIGHT_SOLID_OUTPOST, us, 1);
            }

            if (sqbb & CENTER_FILES_BB)
            {
                ret += KnightCenterOutpost;
                TRACE_ADD(IDX_KNIGHT_CENTER_OUTPOST, us, 1);
            }
        }

        // Give a bonus for a Knight targeting the King Attack zone.
        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += KnightWeight;
            TRACE_ADD(IDX_KS_KNIGHT, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }
    }
    return ret;
}

scorepair_t evaluate_bishops(const Board *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board) ^ piece_bb(board, us, QUEEN);
    bitboard_t bb = piece_bb(board, us, BISHOP);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);

    // Give a bonus for the Bishop pair.
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

        // Give a bonus/penalty based on the number of friendly Pawns which are
        // on same color squares as the Bishop.
        {
            bitboard_t sqMask = (sqbb & DSQ_BB) ? DSQ_BB : LSQ_BB;

            ret += BishopPawnsSameColor[imin(popcount(sqMask & ourPawns), 6)];
            TRACE_ADD(IDX_BISHOP_PAWNS_COLOR + imin(popcount(sqMask & ourPawns), 6), us, 1);
        }

        // If the Bishop is pinned, reduce its mobility to all the squares
        // between the King and the pinner.
        if (board->stack->kingBlockers[us] & sqbb) b &= LineBB[get_king_square(board, us)][sq];

        // Update the attack tables.
        eval->attackedBy[us][BISHOP] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Give a bonus for the Bishop's mobility.
        ret += MobilityB[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_BISHOP + popcount(b & eval->mobilityZone[us]), us, 1);

        // Give a bonus for Bishop with a Pawn above it.
        if (relative_shift_up(square_bb(sq), us) & ourPawns)
        {
            ret += BishopShielded;
            TRACE_ADD(IDX_BISHOP_SHIELDED, us, 1);
        }

        // Give a bonus for a Bishop seeing the center squares (generally in
        // fianchetto).
        if (more_than_one(b & CENTER_BB))
        {
            ret += BishopLongDiagonal;
            TRACE_ADD(IDX_BISHOP_LONG_DIAG, us, 1);
        }

        // Give a bonus for a Bishop targeting the King Attack zone.
        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += BishopWeight;
            TRACE_ADD(IDX_KS_BISHOP, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }
    }
    return ret;
}

scorepair_t evaluate_rooks(const Board *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board) ^ pieces_bb(board, us, ROOK, QUEEN);
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
        // between the King and the pinner.
        if (board->stack->kingBlockers[us] & sqbb) b &= LineBB[get_king_square(board, us)][sq];

        // Update the attack tables.
        eval->attackedBy[us][ROOK] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Give a bonus for a Rook on an open (or semi-open) file.
        if (!(rookFile & ourPawns))
        {
            ret += (rookFile & theirPawns) ? RookOnSemiOpenFile : RookOnOpenFile;
            TRACE_ADD((rookFile & theirPawns) ? IDX_ROOK_SEMIOPEN : IDX_ROOK_OPEN, us, 1);
        }

        // Give a penalty for a Rook on a file with a blocked friendly Pawn.
        else if (relative_shift_up(rookFile & ourPawns, us) & occupancy)
        {
            ret += RookOnBlockedFile;
            TRACE_ADD(IDX_ROOK_BLOCKED, us, 1);
        }

        // Give a bonus for a Rook on the same file as the opponent's Queen(s).
        if (rookFile & theirQueens)
        {
            ret += RookXrayQueen;
            TRACE_ADD(IDX_ROOK_XRAY_QUEEN, us, 1);
        }

        // Give a bonus for the Rook's mobility.
        ret += MobilityR[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_ROOK + popcount(b & eval->mobilityZone[us]), us, 1);

        // Give a bonus for a Rook targeting the King Attack zone.
        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += RookWeight;
            TRACE_ADD(IDX_KS_ROOK, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }
    }
    return ret;
}

scorepair_t evaluate_queens(const Board *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancyB = occupancy_bb(board) ^ piece_bb(board, us, BISHOP);
    const bitboard_t occupancyR = occupancy_bb(board) ^ piece_bb(board, us, ROOK);
    bitboard_t bb = piece_bb(board, us, QUEEN);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancyB) | rook_moves_bb(sq, occupancyR);

        TRACE_ADD(IDX_PIECE + QUEEN - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (QUEEN - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Queen is pinned, reduce its mobility to all the squares
        // between the King and the pinner.
        if (board->stack->kingBlockers[us] & sqbb) b &= LineBB[get_king_square(board, us)][sq];

        // Update the attack tables.
        eval->attackedBy[us][QUEEN] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Give a bonus for the Queen's mobility.

        ret += MobilityQ[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_QUEEN + popcount(b & eval->mobilityZone[us]), us, 1);

        // Give a bonus for a Queen targeting the King Attack zone.
        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += QueenWeight;
            TRACE_ADD(IDX_KS_QUEEN, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }
    }
    return ret;
}

scorepair_t evaluate_threats(const Board *board, const evaluation_t *eval, color_t us)
{
    color_t them = not_color(us);
    bitboard_t rooks = piece_bb(board, them, ROOK);
    bitboard_t queens = piece_bb(board, them, QUEEN);
    scorepair_t ret = 0;
    bitboard_t attacks;

    attacks = pieces_bb(board, them, KNIGHT, BISHOP) & eval->attackedBy[us][PAWN];

    // Give a bonus for a Pawn attacking a minor piece.
    if (attacks)
    {
        ret += PawnAttacksMinor * popcount(attacks);
        TRACE_ADD(IDX_PAWN_ATK_MINOR, us, popcount(attacks));
    }

    attacks = rooks & eval->attackedBy[us][PAWN];

    // Give a bonus for a Pawn attacking a Rook.
    if (attacks)
    {
        rooks &= ~attacks;
        ret += PawnAttacksRook * popcount(attacks);
        TRACE_ADD(IDX_PAWN_ATK_ROOK, us, popcount(attacks));
    }

    attacks = queens & eval->attackedBy[us][PAWN];

    // Give a bonus for a Pawn attacking a Queen.
    if (attacks)
    {
        queens &= ~attacks;
        ret += PawnAttacksQueen * popcount(attacks);
        TRACE_ADD(IDX_PAWN_ATK_QUEEN, us, popcount(attacks));
    }

    attacks = rooks & (eval->attackedBy[us][KNIGHT] | eval->attackedBy[us][BISHOP]);

    // Give a bonus for a minor piece attacking a Rook.
    if (attacks)
    {
        ret += MinorAttacksRook * popcount(attacks);
        TRACE_ADD(IDX_MINOR_ATK_ROOK, us, popcount(attacks));
    }

    attacks = queens & (eval->attackedBy[us][KNIGHT] | eval->attackedBy[us][BISHOP]);

    // Give a bonus for a minor piece attacking a Queen.
    if (attacks)
    {
        queens &= ~attacks;
        ret += MinorAttacksQueen * popcount(attacks);
        TRACE_ADD(IDX_MINOR_ATK_QUEEN, us, popcount(attacks));
    }

    attacks = queens & eval->attackedBy[us][ROOK];

    // Give a bonus for a Rook attacking a Queen.
    if (attacks)
    {
        ret += RookAttacksQueen * popcount(attacks);
        TRACE_ADD(IDX_ROOK_ATK_QUEEN, us, popcount(attacks));
    }

    return ret;
}

scorepair_t evaluate_safety_file(
    bitboard_t ourPawns, bitboard_t theirPawns, file_t file, square_t theirKing, color_t us)
{
    scorepair_t ret = 0;

    // Map the base array index depending on whether the current file is on the
    // side (0), in the middle of the field (16), or the same as the King (8).
    file_t kingFile = sq_file(theirKing);
    int fileIndex = (kingFile == file) ? 8 : (kingFile >= FILE_E) == (kingFile < file) ? 0 : 16;

    // Give a bonus/penalty based on the rank distance between our Pawns and their King.
    {
        bitboard_t pawnMask = ourPawns & file_bb(file);
        int distance =
            pawnMask ? abs(sq_rank(bb_relative_last_sq(us, pawnMask)) - sq_rank(theirKing)) : 7;

        ret += KingStorm[fileIndex + distance];

        TRACE_ADD(IDX_KS_STORM + fileIndex + distance, us, 1);
    }

    // Give a bonus/penalty based on the rank distance between their Pawns and their King.
    {
        bitboard_t pawnMask = theirPawns & file_bb(file);
        int distance =
            pawnMask ? abs(sq_rank(bb_relative_last_sq(us, pawnMask)) - sq_rank(theirKing)) : 7;

        ret += KingShelter[fileIndex + distance];

        TRACE_ADD(IDX_KS_SHELTER + fileIndex + distance, us, 1);
    }

    return ret;
}

scorepair_t evaluate_safety(const Board *board, evaluation_t *eval, color_t us)
{
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone, or
    // one piece attacking with a friendly Queen still on the board.
    bool queenless = !piece_bb(board, us, QUEEN);

    if (eval->safetyAttackers[us] >= 1 + queenless)
    {
        color_t them = not_color(us);
        square_t theirKing = get_king_square(board, them);

        // We define weak squares as squares that we attack where the enemy has
        // no defenders, or only the King as a defender.
        bitboard_t weak = eval->attacked[us] & ~eval->attackedTwice[them]
                          & (~eval->attacked[them] | eval->attackedBy[them][KING]);

        // We define safe squares as squares that are attacked (or weak and attacked twice),
        // and where we can land on.
        bitboard_t safe =
            ~color_bb(board, us) & (~eval->attacked[them] | (weak & eval->attackedTwice[us]));

        bitboard_t rookCheckSpan = rook_moves(board, theirKing);
        bitboard_t bishopCheckSpan = bishop_moves(board, theirKing);

        bitboard_t knightChecks = eval->attackedBy[us][KNIGHT] & knight_moves(theirKing);
        bitboard_t allChecks = knightChecks;

        bitboard_t bishopChecks = eval->attackedBy[us][BISHOP] & bishopCheckSpan;
        allChecks |= bishopChecks;

        bitboard_t rookChecks = eval->attackedBy[us][ROOK] & rookCheckSpan;
        allChecks |= rookChecks;

        bitboard_t queenChecks = eval->attackedBy[us][QUEEN] & (bishopCheckSpan | rookCheckSpan);
        allChecks |= queenChecks;

        scorepair_t bonus = eval->safetyScore[us] + SafetyOffset;

        // Add bonuses based on the number of attacks and the number of weak
        // squares in the King zone, and a penalty if we don't have a Queen for
        // attacking.
        bonus += AttackWeight * eval->safetyAttacks[us];
        bonus += WeakKingZone * popcount(weak & eval->kingZone[us]);
        bonus += QueenlessAttack * queenless;

        // Add bonuses per piece for each safe check that we can perform.
        bonus += SafeKnightCheck * popcount(knightChecks & safe);
        bonus += SafeBishopCheck * popcount(bishopChecks & safe);
        bonus += SafeRookCheck * popcount(rookChecks & safe);
        bonus += SafeQueenCheck * popcount(queenChecks & safe);

        // Add a bonus for all unsafe checks we can perform.
        bonus += UnsafeCheck * popcount(allChecks & ~safe);

        // Evaluate the Pawn Storm/Shelter.
        {
            bitboard_t ourPawns = piece_bb(board, us, PAWN);
            bitboard_t theirPawns = piece_bb(board, them, PAWN);

            for (file_t f = imax(FILE_A, sq_file(theirKing) - 1);
                 f <= imin(FILE_H, sq_file(theirKing) + 1); ++f)
                bonus += evaluate_safety_file(ourPawns, theirPawns, f, theirKing, us);
        }

        TRACE_ADD(IDX_KS_OFFSET, us, 1);
        TRACE_ADD(IDX_KS_QUEENLESS, us, queenless);
        TRACE_ADD(IDX_KS_WEAK_Z, us, popcount(weak & eval->kingZone[us]));
        TRACE_ADD(IDX_KS_CHECK_N, us, popcount(knightChecks & safe));
        TRACE_ADD(IDX_KS_CHECK_B, us, popcount(bishopChecks & safe));
        TRACE_ADD(IDX_KS_CHECK_R, us, popcount(rookChecks & safe));
        TRACE_ADD(IDX_KS_CHECK_Q, us, popcount(queenChecks & safe));
        TRACE_ADD(IDX_KS_UNSAFE_CHECK, us, popcount(allChecks & ~safe));
        TRACE_SAFETY(us, bonus);

        // Compute the final safety score. The middlegame term scales
        // quadratically, while the endgame term scales linearly. The safety
        // evaluation cannot be negative.
        score_t mg = midgame_score(bonus), eg = endgame_score(bonus);

        return create_scorepair(imax(mg, 0) * mg / 256, imax(eg, 0) / 16);
    }

    TRACE_CLEAR_SAFETY(us);
    return 0;
}

score_t evaluate(const Board *board)
{
    TRACE_INIT;

    // Do we have a specialized endgame eval for the current configuration ?
    const EndgameEntry *entry = endgame_probe(board);

    if (entry != NULL) return entry->scoreFunc(board, entry->winningSide);

    // Is there a KXK situation ? (lone King vs mating material)
    if (is_kxk_endgame(board, WHITE)) return eval_kxk(board, WHITE);
    if (is_kxk_endgame(board, BLACK)) return eval_kxk(board, BLACK);

    evaluation_t eval;
    scorepair_t tapered = board->psqScorePair;
    KingPawnEntry *kpe;
    score_t mg, eg, score;

    // Give a bonus for having castling rights in the middlegame.
    // Castled King bonus values more than castling right bonus to incite
    // castling in the opening and quick attacks on an uncastled King.
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

    kpe = kp_probe(board);
    eval_init(board, kpe, &eval);

    // Add the King-Pawn structure evaluation.
    tapered += kpe->value;

    // Add the pieces' evaluation.
    tapered += evaluate_knights(board, &eval, kpe, WHITE);
    tapered -= evaluate_knights(board, &eval, kpe, BLACK);
    tapered += evaluate_bishops(board, &eval, WHITE);
    tapered -= evaluate_bishops(board, &eval, BLACK);
    tapered += evaluate_rooks(board, &eval, WHITE);
    tapered -= evaluate_rooks(board, &eval, BLACK);
    tapered += evaluate_queens(board, &eval, WHITE);
    tapered -= evaluate_queens(board, &eval, BLACK);

    // Add the threats' evaluation.
    tapered += evaluate_threats(board, &eval, WHITE);
    tapered -= evaluate_threats(board, &eval, BLACK);

    // Add the King Safety evaluation.
    tapered += evaluate_safety(board, &eval, WHITE);
    tapered -= evaluate_safety(board, &eval, BLACK);

    // Add the Initiative bonus for the side to move.
    tapered += (board->sideToMove == WHITE) ? Initiative : -Initiative;
    TRACE_ADD(IDX_INITIATIVE, board->sideToMove, 1);

    TRACE_EVAL(tapered);

    mg = midgame_score(tapered);

    // Scale the endgame score based on the remaining material and the Pawns.
    eg = scale_endgame(board, kpe, endgame_score(tapered));

    // Compute the evaluation by interpolating between the middlegame and
    // endgame scores.
    {
        int phase = 4 * popcount(piecetype_bb(board, QUEEN))
                    + 2 * popcount(piecetype_bb(board, ROOK))
                    + popcount(piecetypes_bb(board, KNIGHT, BISHOP));

        phase = iclamp(phase, ENDGAME_COUNT, MIDGAME_COUNT);

        score = mg * (phase - ENDGAME_COUNT) / (MIDGAME_COUNT - ENDGAME_COUNT);
        score += eg * (MIDGAME_COUNT - phase) / (MIDGAME_COUNT - ENDGAME_COUNT);

        TRACE_PHASE(phase);
    }

    // Return the score relative to the side to move.
    return board->sideToMove == WHITE ? score : -score;
}
