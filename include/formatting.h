/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   formatting.h                                     .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:13:28 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 23:41:35 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef FORMATTING_H
# define FORMATTING_H

# if defined(_WIN32) || defined(_WIN64)

// Windows has some weird format strings for size_t

#  define SIZE_FORMAT "%I64u"

# else // Assume standard size_t format

#  define SIZE_FORMAT "%zu"

# endif

#endif
