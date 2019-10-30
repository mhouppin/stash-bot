/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci.h                                            .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 14:13:28 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/30 09:43:42 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#ifndef UCI_H
# define UCI_H

typedef struct	s_cmdlink
{
	const char	*cmd_name;
	void		(*call)(const char *);
}				t_cmdlink;

void	uci_debug(const char *args);
void	uci_go(const char *args);
void	uci_isready(const char *args);
void	uci_position(const char *args);
void	uci_quit(const char *args);
void	uci_setoption(const char *args);
void	uci_stop(const char *args);
void	uci_uci(const char *args);
void	uci_ucinewgame(const char *args);

void	*uci_thread(void *nothing);

#endif
