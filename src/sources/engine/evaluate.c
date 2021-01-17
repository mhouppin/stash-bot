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
#include "info.h"
#include "pawns.h"

enum
{
    CastlingBonus = SPAIR(85, -43),
    Initiative = SPAIR(7, 10),

    KnightWeight = SPAIR(26, 8),
    BishopWeight = SPAIR(18, 5),
    RookWeight = SPAIR(51, -4),
    QueenWeight = SPAIR(51, 72),

    BishopPairBonus = SPAIR(12, 103),
    KnightPairPenalty = SPAIR(-7, 7),
    RookPairPenalty = SPAIR(-39, 24),

    RookOnSemiOpenFile = SPAIR(19, 17),
    RookOnOpenFile = SPAIR(38, 16),
    RookXrayQueen = SPAIR(7, 9),

    QueenPhase = 4,
    RookPhase = 2,
    MinorPhase = 1,

    MidgamePhase = 24,
};

const scorepair_t   MobilityN[9] = {
    SPAIR( -83, -76), SPAIR( -40, -74), SPAIR( -24, -15), SPAIR( -16,  26),
    SPAIR(  -4,  32), SPAIR(  -3,  49), SPAIR(   4,  53), SPAIR(  13,  48),
    SPAIR(  26,  31)
};

const scorepair_t   MobilityB[14] = {
    SPAIR( -96, -80), SPAIR( -51,-100), SPAIR( -16, -66), SPAIR( -15, -24),
    SPAIR(  -5,  -1), SPAIR(   1,  16), SPAIR(   4,  33), SPAIR(   4,  41),
    SPAIR(   4,  49), SPAIR(   5,  54), SPAIR(   8,  52), SPAIR(  19,  44),
    SPAIR(  44,  43), SPAIR(  46,  26)
};

const scorepair_t   MobilityR[15] = {
    SPAIR( -43, -11), SPAIR( -56, -14), SPAIR( -37,  -6), SPAIR( -35,  22),
    SPAIR( -34,  59), SPAIR( -33,  69), SPAIR( -31,  84), SPAIR( -28,  91),
    SPAIR( -24,  94), SPAIR( -19,  98), SPAIR( -15, 104), SPAIR( -13, 105),
    SPAIR(  -8, 105), SPAIR(   7,  95), SPAIR(  54,  64)
};

const scorepair_t   MobilityQ[28] = {
    SPAIR(  -8,-144), SPAIR(  -6,-117), SPAIR(  -5, -90), SPAIR(  -7, -63),
    SPAIR( -13, -38), SPAIR(  -6, -11), SPAIR(   5,  26), SPAIR(   7,  74),
    SPAIR(  12, 101), SPAIR(  15, 125), SPAIR(  19, 137), SPAIR(  21, 159),
    SPAIR(  25, 170), SPAIR(  30, 173), SPAIR(  30, 183), SPAIR(  30, 189),
    SPAIR(  29, 191), SPAIR(  25, 197), SPAIR(  24, 198), SPAIR(  21, 196),
    SPAIR(  33, 186), SPAIR(  33, 183), SPAIR(  31, 171), SPAIR(  31, 167),
    SPAIR(  21, 156), SPAIR(  15, 151), SPAIR(  15, 145), SPAIR(  17, 146)
};

const int   AttackRescale[8] = {
    1, 1, 2, 4, 8, 16, 32, 64
};

typedef struct
{
    bitboard_t  king_zone[COLOR_NB];
    bitboard_t  mobility_zone[COLOR_NB];
    int         attackers[COLOR_NB];
    scorepair_t weights[COLOR_NB];
    int         tempos[COLOR_NB];
}
evaluation_t;

void        eval_init(const board_t *board, evaluation_t *eval)
{
    eval->attackers[WHITE] = eval->attackers[BLACK]
        = eval->weights[WHITE] = eval->weights[BLACK] = 0;

    // Set the King Attack zone as the 3x4 square surrounding the king
    // (counting an additional rank in front of the king)

    eval->king_zone[WHITE] = king_moves(get_king_square(board, BLACK));
    eval->king_zone[BLACK] = king_moves(get_king_square(board, WHITE));
    eval->king_zone[WHITE] |= shift_down(eval->king_zone[WHITE]);
    eval->king_zone[BLACK] |= shift_up(eval->king_zone[BLACK]);

    bitboard_t  occupied = occupancy_bb(board);
    bitboard_t  wpawns = piece_bb(board, WHITE, PAWN);
    bitboard_t  bpawns = piece_bb(board, BLACK, PAWN);
    bitboard_t  wattacks = wpawns_attacks_bb(wpawns);
    bitboard_t  battacks = bpawns_attacks_bb(bpawns);

    // Exclude opponent pawns' attacks from the Mobility and King Attack zones

    eval->mobility_zone[WHITE] = ~battacks;
    eval->mobility_zone[BLACK] = ~wattacks;
    eval->king_zone[WHITE] &= eval->mobility_zone[WHITE];
    eval->king_zone[BLACK] &= eval->mobility_zone[BLACK];

    // Exclude rammed pawns and our pawns on rank 2 and 3 from mobility zone

    eval->mobility_zone[WHITE] &= ~(wpawns & (shift_down(occupied) | RANK_2_BITS | RANK_3_BITS));
    eval->mobility_zone[BLACK] &= ~(bpawns & (shift_up(occupied) | RANK_6_BITS | RANK_7_BITS));

    // Add pawn attacks on opponent's pieces as tempos

    eval->tempos[WHITE] = popcount(color_bb(board, BLACK) & ~piecetype_bb(board, PAWN) & wattacks);
    eval->tempos[BLACK] = popcount(color_bb(board, WHITE) & ~piecetype_bb(board, PAWN) & battacks);

    // If not in check, add one tempo to the side to move

    eval->tempos[board->side_to_move] += !board->stack->checkers;
}

scorepair_t evaluate_knights(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t ret = 0;
    bitboard_t  bb = piece_bb(board, c, KNIGHT);
    bitboard_t  targets = pieces_bb(board, not_color(c), ROOK, QUEEN);

    // Penalty for having the Knight pair

    if (more_than_one(bb))
        ret += KnightPairPenalty;

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  b = knight_moves(sq);

        // Bonus for Knight mobility

        ret += MobilityN[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Knight on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * KnightWeight;
        }

        // Tempo bonus for a Knight attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[c] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_bishops(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board);
    bitboard_t          bb = piece_bb(board, c, BISHOP);
    bitboard_t          targets = pieces_bb(board, not_color(c), ROOK, QUEEN);

    // Bonus for the Bishop pair

    if (more_than_one(bb))
        ret += BishopPairBonus;

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  b = bishop_moves_bb(sq, occupancy);

        // Bonus for Bishop mobility

        ret += MobilityB[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Bishop on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * BishopWeight;
        }

        // Tempo bonus for a Bishop attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[c] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_rooks(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board);
    const bitboard_t    my_pawns = piece_bb(board, c, PAWN);
    const bitboard_t    their_pawns = piece_bb(board, not_color(c), PAWN);
    const bitboard_t    their_queens = piece_bb(board, not_color(c), QUEEN);
    bitboard_t          bb = piece_bb(board, c, ROOK);

    // Penalty for the Rook pair

    if (more_than_one(bb))
        ret += RookPairPenalty;

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  rook_file = sq_file_bb(sq);
        bitboard_t  b = rook_moves_bb(sq, occupancy);

        // Bonus for a Rook on an open (or semi-open) file

        if (!(rook_file & my_pawns))
            ret += (rook_file & their_pawns) ? RookOnSemiOpenFile : RookOnOpenFile;

        // Bonus for a Rook on the same file as the opponent's Queen(s)

        if (rook_file & their_queens)
            ret += RookXrayQueen;

        // Bonus for Rook mobility

        ret += MobilityR[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Rook on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * RookWeight;
        }

        // Tempo bonus for a Rook attacking the opponent's Queen(s)

        if (b & their_queens)
            eval->tempos[c] += popcount(b & their_queens);
    }
    return (ret);
}

scorepair_t evaluate_queens(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board);
    bitboard_t          bb = piece_bb(board, c, QUEEN);

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  b = bishop_moves_bb(sq, occupancy) | rook_moves_bb(sq, occupancy);

        // Bonus for Queen mobility

        ret += MobilityQ[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Queen on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * QueenWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_safety(evaluation_t *eval, color_t c)
{
    scorepair_t bonus = eval->weights[c];

    // Add a bonus if we have 2 pieces (or more) on the King Attack zone

    if (eval->attackers[c] < 8)
        bonus -= scorepair_divide(bonus, AttackRescale[eval->attackers[c]]);

    return (bonus);
}

score_t evaluate(const board_t *board)
{
    evaluation_t    eval;
    scorepair_t     tapered = board->psq_scorepair;

    // Bonus for having castling rights in the middlegame
    // (castled King bonus values more than castling right bonus to incite
    // castling in the opening and quick attacks on an uncastled King)

    if (board->stack->castlings & WHITE_CASTLING)
        tapered += CastlingBonus;
    if (board->stack->castlings & BLACK_CASTLING)
        tapered -= CastlingBonus;

    eval_init(board, &eval);

    // Add the pawn structure evaluation

    tapered += evaluate_pawns(board);

    // Add the pieces' evaluation

    tapered += evaluate_knights(board, &eval, WHITE);
    tapered -= evaluate_knights(board, &eval, BLACK);
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

    score_t mg = midgame_score(tapered);
    score_t eg = endgame_score(tapered);
    score_t score;

    // Scale endgame score based on remaining material + pawns

    eg = scale_endgame(board, eg);

    // Compute the eval by interpolating between the middlegame and endgame scores

    {
        int phase = QueenPhase * popcount(board->piecetype_bits[QUEEN])
            + RookPhase * popcount(board->piecetype_bits[ROOK])
            + MinorPhase * popcount(board->piecetype_bits[KNIGHT] | board->piecetype_bits[BISHOP]);

        if (phase >= MidgamePhase)
            score = mg;
        else
        {
            score = mg * phase / MidgamePhase;
            score += eg * (MidgamePhase - phase) / MidgamePhase;
        }
    }

    // Return the score relative to the side to move

    return (board->side_to_move == WHITE ? score : -score);
}
