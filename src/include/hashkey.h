/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   hashkey.h                                        .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 15:54:21 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/19 18:18:19 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef HASHKEY_H
# define HASHKEY_H

# include "castling.h"
# include "piece.h"
# include "square.h"

typedef uint64_t	hashkey_t;

extern hashkey_t	ZobristPsq[PIECE_NB][SQUARE_NB];
extern hashkey_t	ZobristEnPassant[FILE_NB];
extern hashkey_t	ZobristCastling[CASTLING_NB];
extern hashkey_t	ZobristBlackToMove;

#endif
