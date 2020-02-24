/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   inlining.h                                       .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/18 14:34:29 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/18 14:36:33 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef INLINING_H
# define INLINING_H

// Workaround to avoid problems with duplicated function symbols,
// when -finline is disabled or when a function inlining fails.

# define INLINED	static inline

#endif
