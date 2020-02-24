/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   init.h                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/19 16:48:52 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/22 18:17:53 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef INIT_H
# define INIT_H

void	*uci_thread(void *nothing);
void	*engine_thread(void *nothing);

void	bitboard_init(void);
void	psq_score_init(void);
void	zobrist_init(void);

#endif
