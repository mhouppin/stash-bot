/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2023 Morgan Houppin
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

#include "endgame.h"
#include "pawns.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EndgameEntry EndgameTable[EGTB_SIZE];

score_t eval_draw(
    const Board *board __attribute__((unused)), color_t winningSide __attribute__((unused)))
{
    return 0;
}

score_t eval_krkp(const Board *board, color_t winningSide)
{
    square_t winningKsq = get_king_square(board, winningSide);
    square_t losingKsq = get_king_square(board, not_color(winningSide));
    square_t winningRook = bb_first_sq(piecetype_bb(board, ROOK));
    square_t losingPawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t pushSquare = losingPawn + pawn_direction(not_color(winningSide));
    square_t promoteSquare =
        create_sq(sq_file(losingPawn), relative_rank(RANK_8, not_color(winningSide)));
    score_t score;

    // Does the winning King control the promotion path ?
    if (forward_file_bb(winningSide, winningKsq) & square_bb(losingPawn))
        score = ROOK_EG_SCORE - SquareDistance[winningKsq][losingPawn];

    // Is the losing King unable to reach either the Rook or its Pawn ?
    else if (SquareDistance[losingKsq][losingPawn] >= 3 + (board->sideToMove != winningSide)
             && SquareDistance[losingKsq][winningRook] >= 3)
        score = ROOK_EG_SCORE - SquareDistance[winningKsq][losingPawn];

    // Is the Pawn close to the promotion square and defended by its King ?
    // Also, is the winning King unable to reach the Pawn ?
    else if (relative_rank(winningSide, losingKsq) <= RANK_3
             && SquareDistance[losingKsq][losingPawn] == 1
             && relative_rank(winningSide, winningKsq) >= RANK_4
             && SquareDistance[winningKsq][losingPawn] >= 3 + (board->sideToMove == winningSide))
        score = 40 - 4 * SquareDistance[winningKsq][losingPawn];

    else
        score =
            100
            - 4
                  * (SquareDistance[winningKsq][pushSquare] - SquareDistance[losingKsq][pushSquare]
                      - SquareDistance[losingPawn][promoteSquare]);

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_krkn(const Board *board, color_t winningSide)
{
    square_t losingKsq = get_king_square(board, not_color(winningSide));
    square_t losingKnight = bb_first_sq(piecetype_bb(board, KNIGHT));
    score_t score = edge_bonus(losingKsq) + away_bonus(losingKsq, losingKnight);

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_krkb(const Board *board, color_t winningSide)
{
    score_t score = edge_bonus(get_king_square(board, not_color(winningSide)));

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_kbnk(const Board *board, color_t winningSide)
{
    square_t losingKsq = get_king_square(board, not_color(winningSide));
    square_t winningKsq = get_king_square(board, winningSide);
    score_t score = VICTORY + KNIGHT_MG_SCORE + BISHOP_MG_SCORE;

    score += 70 - 10 * SquareDistance[losingKsq][winningKsq];

    // Don't push the King to the wrong corner.
    if (piecetype_bb(board, BISHOP) & DSQ_BB) losingKsq ^= SQ_A8;

    score += abs(sq_file(losingKsq) - sq_rank(losingKsq)) * 100;

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_kqkr(const Board *board, color_t winningSide)
{
    score_t score = QUEEN_EG_SCORE - ROOK_EG_SCORE;
    square_t losingKsq = get_king_square(board, not_color(winningSide));
    square_t winningKsq = get_king_square(board, winningSide);

    score += edge_bonus(losingKsq);
    score += close_bonus(winningKsq, losingKsq);

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_kqkp(const Board *board, color_t winningSide)
{
    square_t losingKsq = get_king_square(board, not_color(winningSide));
    square_t losingPawn = bb_first_sq(piecetype_bb(board, PAWN));
    square_t winningKsq = get_king_square(board, winningSide);
    score_t score = close_bonus(winningKsq, losingKsq);

    // Check for potential draws with a Pawn on its 7th Rank on either a Knight
    // or Rook file, and the winning King too far away to avoid the promotion
    // without perpetual checks.
    if (relative_sq_rank(losingPawn, not_color(winningSide)) != RANK_7
        || SquareDistance[losingKsq][losingPawn] != 1
        || ((FILE_B_BB | FILE_D_BB | FILE_E_BB | FILE_G_BB) & square_bb(losingPawn)))
        score += QUEEN_EG_SCORE - PAWN_EG_SCORE;

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_knnkp(const Board *board, color_t winningSide)
{
    score_t score = PAWN_EG_SCORE;

    // Very complex position. For mating, we need the Pawn sufficiently far away
    // from its promotion square, and the losing King close to an edge.
    score += edge_bonus(get_king_square(board, not_color(winningSide)));
    score -= 5 * relative_sq_rank(bb_first_sq(piecetype_bb(board, PAWN)), not_color(winningSide));

    return board->sideToMove == winningSide ? score : -score;
}

score_t eval_kmpkn(const Board *board, color_t winningSide)
{
    PawnEntry *pe = pawn_probe(board);
    score_t score = endgame_score(pe->value + board->psqScorePair);
    color_t losingSide = not_color(winningSide);
    bool tempo = (board->sideToMove == losingSide);

    square_t winningKsq = get_king_square(board, winningSide);
    square_t winningPsq = bb_first_sq(piecetype_bb(board, PAWN));
    square_t losingKsq = get_king_square(board, losingSide);
    square_t losingNsq = bb_first_sq(piece_bb(board, losingSide, KNIGHT));

    if (board->sideToMove == BLACK) score = -score;

    bitboard_t queeningPath = forward_file_bb(winningSide, winningPsq);
    bitboard_t passedSpan = passed_pawn_span_bb(winningSide, winningPsq);

    // Verifications are simple here: if either the defending King or Knight can
    // control efficiently the promotion path of the Pawn, the position is very
    // drawish.
    if (queeningPath & square_bb(losingKsq)) return score / 16;

    if (queeningPath & knight_moves(losingNsq)) return score / 8;

    if ((passedSpan & square_bb(losingKsq)) && !(passedSpan & square_bb(winningKsq)) && tempo)
        return score / 4;

    return score;
}

score_t eval_kmpkb(const Board *board, color_t winningSide)
{
    PawnEntry *pe = pawn_probe(board);
    score_t score = endgame_score(pe->value + board->psqScorePair);
    color_t losingSide = not_color(winningSide);
    bool tempo = (board->sideToMove == losingSide);

    square_t winningKsq = get_king_square(board, winningSide);
    square_t winningPsq = bb_first_sq(piecetype_bb(board, PAWN));
    square_t losingKsq = get_king_square(board, losingSide);
    square_t losingBsq = bb_first_sq(piece_bb(board, losingSide, BISHOP));

    if (board->sideToMove == BLACK) score = -score;

    bitboard_t queeningPath = forward_file_bb(winningSide, winningPsq);
    bitboard_t passedSpan = passed_pawn_span_bb(winningSide, winningPsq);

    // Verifications are simple here: if either the defending King or Bishop can
    // control efficiently the promotion path of the Pawn, the position is very
    // drawish.
    if (queeningPath & square_bb(losingKsq)) return score / 16;

    if (queeningPath & bishop_moves(board, losingBsq)) return score / 8;

    if ((passedSpan & square_bb(losingKsq)) && !(passedSpan & square_bb(winningKsq)) && tempo)
        return score / 4;

    return score;
}

score_t eval_krpkr(const Board *board, color_t winningSide)
{
    PawnEntry *pe = pawn_probe(board);
    score_t score = endgame_score(pe->value + board->psqScorePair);
    color_t losingSide = not_color(winningSide);
    bool tempo = (board->sideToMove == losingSide);

    square_t winningKsq = relative_sq(get_king_square(board, winningSide), winningSide);
    square_t winningPsq = relative_sq(bb_first_sq(piecetype_bb(board, PAWN)), winningSide);
    square_t losingKsq = relative_sq(get_king_square(board, losingSide), winningSide);
    square_t losingRsq = relative_sq(bb_first_sq(piece_bb(board, losingSide, ROOK)), winningSide);

    if (board->sideToMove == BLACK) score = -score;

    // Check if a Philidor position can be built, or if the back-rank defense
    // can be used.
    if (passed_pawn_span_bb(WHITE, winningPsq) & square_bb(losingKsq))
    {
        rank_t rankLK = sq_rank(losingKsq);
        rank_t rankLR = sq_rank(losingRsq);
        rank_t rankWK = sq_rank(winningKsq);
        rank_t rankWP = sq_rank(winningPsq);

        if (rankLK == RANK_6 || rankLK == RANK_7)
        {
            if (rankLR == RANK_5 && rankWK < RANK_5 && rankWP < RANK_5) return score / 64;
            if (rankLR == RANK_1 && rankWP == RANK_5) return score / 64;
        }
        if (rankLK >= RANK_7)
        {
            if (rankLR == RANK_6 && rankWK < RANK_6 && rankWP < RANK_6) return score / 64;
            if (rankLR <= RANK_2 && rankWP == RANK_6) return score / 64;
        }

        if (rankLK == RANK_8 && rankLR == RANK_8)
        {
            if (square_bb(winningPsq) & (FILE_A_BB | FILE_B_BB | FILE_G_BB | FILE_H_BB))
                return score / 16;
            if (rankWK + tempo < RANK_6) return score / 16;
        }

        return score / 2;
    }

    return score;
}

int scale_kpsk(const Board *board, color_t winningSide)
{
    const color_t losingSide = not_color(winningSide);
    square_t winningKsq = relative_sq(get_king_square(board, winningSide), winningSide);
    square_t losingKsq = relative_sq(get_king_square(board, losingSide), winningSide);
    const bitboard_t winningPawns = piece_bb(board, winningSide, PAWN);

    if (!winningPawns)
        return SCALE_DRAW;

    // We only check for draw scenarios with all pawns on a Rook file with the
    // defending King on the promotion path.
    if ((winningPawns & FILE_A_BB) == winningPawns || (winningPawns & FILE_H_BB) == winningPawns)
    {
        square_t mostAdvancedPawn =
            (winningSide == WHITE) ? bb_last_sq(winningPawns) : bb_first_sq(winningPawns) ^ SQ_A8;

        // Don't forget to map the pieces to the queenside for a kingside Pawn.
        if (sq_file(mostAdvancedPawn) == FILE_H)
        {
            winningKsq ^= FILE_H;
            losingKsq ^= FILE_H;
            mostAdvancedPawn ^= FILE_H;
        }

        color_t us = (board->sideToMove == winningSide) ? WHITE : BLACK;

        // If a KPK situation with only the most advanced Pawn results in a
        // draw, then the position is likely a draw as well.
        if (!kpk_is_winning(us, losingKsq, winningKsq, mostAdvancedPawn)) return SCALE_DRAW;
    }

    return SCALE_NORMAL;
}

int scale_kbpsk(const Board *board, color_t winningSide)
{
    // If the winning side does not have the bishop, don't try to scale down the endgame.
    if (!piece_bb(board, winningSide, BISHOP))
        return SCALE_NORMAL;

    const color_t losingSide = not_color(winningSide);
    const square_t winningBsq = relative_sq(bb_first_sq(piecetype_bb(board, BISHOP)), winningSide);
    const square_t losingKsq = relative_sq(get_king_square(board, losingSide), winningSide);
    const bitboard_t winningPawns = piece_bb(board, winningSide, PAWN);
    const bitboard_t wrongFile = (square_bb(winningBsq) & DSQ_BB) ? FILE_A_BB : FILE_H_BB;

    if (!winningPawns)
        return SCALE_DRAW;

    // Check for a wrong-colored bishop situation.
    if ((winningPawns & wrongFile) == winningPawns)
    {
        const bitboard_t queeningSquare = (square_bb(winningBsq) & DSQ_BB) ? SQ_A8 : SQ_H8;
        const int queeningDistance = SquareDistance[losingKsq][queeningSquare];

        // Check if the losing King is in control of the queening square.
        if (queeningDistance < 2) return SCALE_DRAW;

        return SCALE_NORMAL * (queeningDistance - 1) / queeningDistance;
    }

    return SCALE_NORMAL;
}

static EndgameEntry *material_to_entry(
    const char *wpieces, const char *bpieces, hashkey_t *materialKey)
{
    char bcopy[16];
    char fen[128];
    size_t i;
    Board board;
    Boardstack stack;

    for (i = 0; bpieces[i]; ++i) bcopy[i] = tolower(bpieces[i]);

    bcopy[i] = '\0';

    // Generate a FEN corresponding to the material distribution of the pieces.
    sprintf(fen, "8/%s%d/8/8/8/8/%s%d/8 w - - 0 1", bcopy, 8 - (int)i, wpieces,
        8 - (int)strlen(wpieces));

    board_from_fen(&board, fen, false, &stack);

    // Map the board to a new endgame entry.
    *materialKey = board.stack->materialKey;
    return &EndgameTable[*materialKey & (EGTB_SIZE - 1)];
}

void add_scoring_colored_entry(
    color_t winningSide, char *wpieces, char *bpieces, endgame_score_func_t scoreFunc)
{
    hashkey_t materialKey;
    EndgameEntry *entry = material_to_entry(wpieces, bpieces, &materialKey);

    // Check for key conflicts and stop the program here.
    if (entry->scoreFunc != NULL)
    {
        fputs("Error: key conflicts in endgame table\n", stderr);
        exit(EXIT_FAILURE);
    }

    // Assign the endgame specifications to the entry.
    entry->key = materialKey;
    entry->scoreFunc = scoreFunc;
    entry->winningSide = winningSide;
}

void add_scoring_entry(const char *pieces, endgame_score_func_t scoreFunc)
{
    char buf[32];

    strcpy(buf, pieces);

    char *split = strchr(buf, 'v');
    *split = '\0';

    // Add the endgame entry for both sides, or one side if material is identical.
    add_scoring_colored_entry(WHITE, buf, split + 1, scoreFunc);

    if (strcmp(buf, split + 1) != 0) add_scoring_colored_entry(BLACK, split + 1, buf, scoreFunc);
}

void add_scaling_colored_entry(
    color_t winningSide, char *wpieces, char *bpieces, endgame_scale_func_t scaleFunc)
{
    hashkey_t materialKey;
    EndgameEntry *entry = material_to_entry(wpieces, bpieces, &materialKey);

    // Check for key conflicts and stop the program here.
    if (entry->scaleFunc != NULL)
    {
        fputs("Error: key conflicts in endgame table\n", stderr);
        exit(EXIT_FAILURE);
    }

    // Assign the endgame specifications to the entry.
    entry->key = materialKey;
    entry->scaleFunc = scaleFunc;
    entry->winningSide = winningSide;
}

void add_scaling_entry(const char *nonPawnMat, endgame_scale_func_t scaleFunc)
{
    char buf[32];

    strcpy(buf, nonPawnMat);

    char *split = strchr(buf, 'v');
    *split = '\0';

    // Add the endgame entry for both sides, or one side if material is identical.
    add_scaling_colored_entry(WHITE, buf, split + 1, scaleFunc);

    if (strcmp(buf, split + 1) != 0) add_scaling_colored_entry(BLACK, split + 1, buf, scaleFunc);
}

void init_endgame_table(void)
{
    static const char *drawnEndgames[] = {
        "KvK", "KNvK", "KBvK", "KNNvK", "KBvKN", "KNvKN", "KBvKB", "KBBvKB", NULL};

    memset(EndgameTable, 0, sizeof(EndgameTable));

    for (size_t i = 0; drawnEndgames[i] != NULL; ++i)
        add_scoring_entry(drawnEndgames[i], &eval_draw);

    // 3-man endgames
    add_scoring_entry("KPvK", &eval_kpk);

    // 4-man endgames
    add_scoring_entry("KRvKP", &eval_krkp);
    add_scoring_entry("KRvKN", &eval_krkn);
    add_scoring_entry("KRvKB", &eval_krkb);
    add_scoring_entry("KQvKR", &eval_kqkr);
    add_scoring_entry("KQvKP", &eval_kqkp);
    add_scoring_entry("KBNvK", &eval_kbnk);

    // 5-man endgames
    add_scoring_entry("KNNvKP", &eval_knnkp);
    add_scoring_entry("KRPvKR", &eval_krpkr);
    add_scoring_entry("KNPvKN", &eval_kmpkn);
    add_scoring_entry("KBPvKN", &eval_kmpkn);
    add_scoring_entry("KNPvKB", &eval_kmpkb);
    add_scoring_entry("KBPvKB", &eval_kmpkb);

    // Scaled endgames
    add_scaling_entry("KvK", &scale_kpsk);
    add_scaling_entry("KBvK", &scale_kbpsk);
}

const EndgameEntry *endgame_probe(const Board *board)
{
    EndgameEntry *entry = &EndgameTable[board->stack->materialKey & (EGTB_SIZE - 1)];

    // Return the entry if the keys match.
    return entry->key == board->stack->materialKey && entry->scoreFunc != NULL ? entry : NULL;
}

const EndgameEntry *endgame_probe_scalefactor(const Board *board)
{
    // Scaling function entries are stored pawnless.
    hashkey_t materialKey = board->stack->materialKey;

    for (int i = 0; i < board->pieceCount[WHITE_PAWN]; ++i)
        materialKey ^= ZobristPsq[WHITE_PAWN][i];

    for (int i = 0; i < board->pieceCount[BLACK_PAWN]; ++i)
        materialKey ^= ZobristPsq[BLACK_PAWN][i];

    EndgameEntry *entry = &EndgameTable[materialKey & (EGTB_SIZE - 1)];

    // Return the entry if the keys match.
    return entry->key == materialKey && entry->scaleFunc != NULL ? entry : NULL;
}
