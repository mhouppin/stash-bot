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
#include "info.h"
#include "pawns.h"

enum
{
    CastlingBonus = SPAIR(85, -43),
    Initiative = SPAIR(10, 15),

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
    bitboard_t  safe_zone[COLOR_NB];
    int         attackers[COLOR_NB];
    scorepair_t weights[COLOR_NB];
}
evaluation_t;

void        eval_init(const board_t *board, evaluation_t *eval)
{
    const bitboard_t    pawns = board->piecetype_bits[PAWN];

    eval->attackers[WHITE] = eval->attackers[BLACK]
        = eval->weights[WHITE] = eval->weights[BLACK] = 0;

    eval->king_zone[WHITE] = king_moves(board_king_square(board, BLACK));
    eval->king_zone[BLACK] = king_moves(board_king_square(board, WHITE));
    eval->king_zone[WHITE] |= shift_down(eval->king_zone[WHITE]);
    eval->king_zone[BLACK] |= shift_up(eval->king_zone[BLACK]);
    eval->safe_zone[WHITE] = ~black_pawn_attacks(pawns & board->color_bits[BLACK]);
    eval->safe_zone[BLACK] = ~white_pawn_attacks(pawns & board->color_bits[WHITE]);
    eval->king_zone[WHITE] &= eval->safe_zone[WHITE];
    eval->king_zone[BLACK] &= eval->safe_zone[BLACK];
}

scorepair_t evaluate_knights(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t ret = 0;
    bitboard_t  bb = piece_bb(board, c, KNIGHT);

    if (more_than_one(bb))
        ret += KnightPairPenalty;

    while (bb)
    {
        square_t    sq = pop_first_square(&bb);
        bitboard_t  b = knight_moves(sq);

        ret += MobilityN[popcount(b & eval->safe_zone[c])];

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * KnightWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_bishops(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = board->piecetype_bits[ALL_PIECES];
    bitboard_t          bb = piece_bb(board, c, BISHOP);

    if (more_than_one(bb))
        ret += BishopPairBonus;

    while (bb)
    {
        square_t    sq = pop_first_square(&bb);
        bitboard_t  b = bishop_move_bits(sq, occupancy);

        ret += MobilityB[popcount(b & eval->safe_zone[c])];

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * BishopWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_rooks(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = board->piecetype_bits[ALL_PIECES];
    const bitboard_t    my_pawns = piece_bb(board, c, PAWN);
    const bitboard_t    their_pawns = piecetype_bb(board, PAWN) & ~my_pawns;
    const bitboard_t    their_queens = piece_bb(board, not_color(c), QUEEN);
    bitboard_t          bb = piece_bb(board, c, ROOK);

    if (more_than_one(bb))
        ret += RookPairPenalty;

    while (bb)
    {
        square_t    sq = pop_first_square(&bb);
        bitboard_t  rook_file = file_square_bits(sq);
        bitboard_t  b = rook_move_bits(sq, occupancy);

        if (!(rook_file & my_pawns))
            ret += (rook_file & their_pawns) ? RookOnSemiOpenFile : RookOnOpenFile;

        if (rook_file & their_queens)
            ret += RookXrayQueen;

        ret += MobilityR[popcount(b & eval->safe_zone[c])];

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * RookWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_queens(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = board->piecetype_bits[ALL_PIECES];
    bitboard_t          bb = piece_bb(board, c, QUEEN);

    while (bb)
    {
        square_t    sq = pop_first_square(&bb);
        bitboard_t  b = bishop_move_bits(sq, occupancy)
            | rook_move_bits(sq, occupancy);

        ret += MobilityQ[popcount(b & eval->safe_zone[c])];

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

    if (eval->attackers[c] < 8)
        bonus -= scorepair_divide(bonus, AttackRescale[eval->attackers[c]]);

    return (bonus);
}

score_t evaluate(const board_t *board)
{
    evaluation_t    eval;
    scorepair_t     tapered = board->psq_scorepair;

    if (board->stack->castlings & WHITE_CASTLING)
        tapered += CastlingBonus;
    if (board->stack->castlings & BLACK_CASTLING)
        tapered -= CastlingBonus;

    eval_init(board, &eval);

    tapered += evaluate_pawns(board);

    tapered += evaluate_knights(board, &eval, WHITE);
    tapered -= evaluate_knights(board, &eval, BLACK);
    tapered += evaluate_bishops(board, &eval, WHITE);
    tapered -= evaluate_bishops(board, &eval, BLACK);
    tapered += evaluate_rooks(board, &eval, WHITE);
    tapered -= evaluate_rooks(board, &eval, BLACK);
    tapered += evaluate_queens(board, &eval, WHITE);
    tapered -= evaluate_queens(board, &eval, BLACK);

    tapered += evaluate_safety(&eval, WHITE);
    tapered -= evaluate_safety(&eval, BLACK);
    tapered += (board->side_to_move == WHITE) ? Initiative : -Initiative;

    score_t mg = midgame_score(tapered);
    score_t eg = endgame_score(tapered);
    int     piece_count = popcount(board->piecetype_bits[ALL_PIECES]);
    score_t score;

    if (piece_count <= 7)
    {
        // Insufficient material check.

        int pieces = popcount(board->color_bits[WHITE]);

        if (eg > 0)
        {
            if (pieces == 1)
                return (0);
            else if (pieces == 2 && pieces_bb(board, WHITE, KNIGHT, BISHOP))
                return (0);
        }

        pieces = piece_count - pieces;

        if (eg < 0)
        {
            if (pieces == 1)
                return (0);
            else if (pieces == 2 && pieces_bb(board, BLACK, KNIGHT, BISHOP))
                return (0);
        }
    }

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

    return (board->side_to_move == WHITE ? score : -score);
}
