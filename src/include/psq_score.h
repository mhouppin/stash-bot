/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   psq_score.h                                      .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:01:58 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 16:52:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef PSQ_SCORE_H
# define PSQ_SCORE_H

# include "piece.h"
# include "score.h"
# include "square.h"

enum
{
	PAWN_MG_SCORE = 100,
	KNIGHT_MG_SCORE = 300,
	BISHOP_MG_SCORE = 330,
	ROOK_MG_SCORE = 500,
	QUEEN_MG_SCORE = 900,

	PAWN_EG_SCORE = 200,
	KNIGHT_EG_SCORE = 600,
	BISHOP_EG_SCORE = 660,
	ROOK_EG_SCORE = 1000,
	QUEEN_EG_SCORE = 1800
};

extern score_t		PieceScores[PHASE_NB][PIECE_NB];
extern scorepair_t	PsqScore[PIECE_NB][SQUARE_NB];

#endif
