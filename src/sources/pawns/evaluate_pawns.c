#include "pawns.h"

enum
{
	BackwardPenalty = SPAIR(-16, -40),
	StragglerPenalty = SPAIR(-24, -56),

	CandidateBonus = SPAIR(20, 48),
	PassedPawnBonus = SPAIR(28, 64),
	PassedRankBonus = SPAIR(32, 56)
};

pawns_cache_t	g_pawns[PawnCacheSize];

bitboard_t	safe_pawn_squares(color_t c, bitboard_t us, bitboard_t them)
{
	bitboard_t	our_double_attacks;
	bitboard_t	our_odd_attacks;
	bitboard_t	their_attacks;
	bitboard_t	their_double_attacks;
	
	if (c == WHITE)
	{
		our_double_attacks = white_pawn_dattacks(us);
		our_odd_attacks = shift_up_left(us) ^ shift_up_right(us);
		their_attacks = black_pawn_attacks(them);
		their_double_attacks = black_pawn_dattacks(them);
	}
	else
	{
		our_double_attacks = black_pawn_dattacks(us);
		our_odd_attacks = shift_down_left(us) ^ shift_down_right(us);
		their_attacks = white_pawn_attacks(them);
		their_double_attacks = white_pawn_dattacks(them);
	}

	return (our_double_attacks | ~their_attacks | (our_odd_attacks & ~their_double_attacks));
}

scorepair_t	evaluate_passers(color_t c, const square_t *ulist, bitboard_t them)
{
	scorepair_t	ret = 0;

	for (square_t sq = *ulist; sq != SQ_NONE; sq = *++ulist)
	{
		rank_t	r = relative_square_rank(sq, c);
		if (r == RANK_7)
			ret += PassedPawnBonus + PassedRankBonus;
		else if (!(passed_pawn_span(c, sq) & them))
			ret += PassedPawnBonus + scorepair_divide(PassedRankBonus, RANK_8 - r);
	}
	return (ret);
}

scorepair_t	evaluate_candidates(color_t c, bitboard_t us, bitboard_t them)
{
	bitboard_t	rank5 = (c == WHITE) ? RANK_5_BITS : RANK_4_BITS;
	bitboard_t	their_attacks = (c == WHITE) ? black_pawn_attacks(them)
		: white_pawn_attacks(them);

	bitboard_t	our_safe_squares = safe_pawn_squares(c, us, them);
	bitboard_t	safe_attacked = our_safe_squares & their_attacks;
	bitboard_t	their_front_span = (c == WHITE) ? (them >> 8) | (them >> 16)
		: (them << 8) | (them << 16);

	bitboard_t	candidates = us & rank5 & ~their_front_span
		& (c == WHITE ? shift_up(safe_attacked) : shift_down(safe_attacked));

	return (candidates ? CandidateBonus * popcount(candidates) : 0);
}

scorepair_t	evaluate_backward(color_t c, bitboard_t us, bitboard_t them,
			const square_t *ulist, const square_t *tlist)
{
	bitboard_t	stops = (c == WHITE) ? shift_up(us) : shift_down(us);
	bitboard_t	our_attack_spans = 0;

	for (square_t sq = *ulist; sq != SQ_NONE; sq = *++ulist)
		our_attack_spans |= pawn_attack_span(c, sq);

	bitboard_t	their_attacks = (c == WHITE)
		? black_pawn_attacks(them) : white_pawn_attacks(them);

	bitboard_t	backward = (stops & their_attacks & ~our_attack_spans);
	backward = (c == WHITE) ? shift_down(backward) : shift_up(backward);

	scorepair_t	ret = 0;

	if (!backward)
		return (ret);

	ret += BackwardPenalty * popcount(backward);

	backward &= (c == WHITE) ? (RANK_2_BITS | RANK_3_BITS) : (RANK_6_BITS | RANK_7_BITS);

	if (!backward)
		return (ret);

	bitboard_t	their_files = 0;

	for (square_t sq = *tlist; sq != SQ_NONE; sq = *++tlist)
		their_files |= forward_file_bits(opposite_color(c), sq);

	backward &= ~their_files;

	if (!backward)
		return (ret);

	ret += StragglerPenalty * popcount(backward);

	return (ret);
}

scorepair_t	evaluate_pawns(const board_t *board)
{
	pawns_cache_t	*entry = &(g_pawns[board->stack->pawn_key
		& (PawnCacheSize - 1)]);

	if (entry->key == board->stack->pawn_key)
		return (entry->value);

	entry->key = board->stack->pawn_key;
	entry->value = 0;

	const bitboard_t	wpawns = board->piecetype_bits[PAWN]
		& board->color_bits[WHITE];
	const bitboard_t	bpawns = board->piecetype_bits[PAWN]
		& board->color_bits[BLACK];
	const square_t		*wlist = board->piece_list[WHITE_PAWN];
	const square_t		*blist = board->piece_list[BLACK_PAWN];

	entry->value += evaluate_backward(WHITE, wpawns, bpawns, wlist, blist);
	entry->value -= evaluate_backward(BLACK, bpawns, wpawns, blist, wlist);
	entry->value += evaluate_passers(WHITE, wlist, bpawns);
	entry->value -= evaluate_passers(BLACK, blist, wpawns);
	entry->value += evaluate_candidates(WHITE, wpawns, bpawns);
	entry->value -= evaluate_candidates(BLACK, bpawns, wpawns);

	return (entry->value);
}
