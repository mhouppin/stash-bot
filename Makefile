# **************************************************************************** #
#                                                           LE - /             #
#                                                               /              #
#    Makefile                                         .::    .:/ .      .::    #
#                                                  +:+:+   +:    +:  +:+:+     #
#    By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+      #
#                                                  #+#   #+    #+    #+#       #
#    Created: 2019/10/28 13:18:56 by mhouppin     #+#   ##    ##    #+#        #
#    Updated: 2019/11/30 14:11:09 by stash       ###    #+. /#+    ###.fr      #
#                                                          /                   #
#                                                         /                    #
# **************************************************************************** #

NAME	:= stash-bot

SOURCES	:= \
	sources/misc/globals.c \
	sources/main/main.c \
	sources/engine/movelist_init.c \
	sources/engine/analysis_thread.c \
	sources/engine/engine_thread.c \
	sources/engine/str_to_move.c \
	sources/engine/movelist_quit.c \
	sources/engine/move_to_str.c \
	sources/engine/launch_analyse.c \
	sources/engine/is_checked.c \
	sources/engine/do_move.c \
	sources/engine/get_simple_moves.c \
	sources/engine/get_piece_moves.c \
	sources/engine/push_move.c \
	sources/engine/pop_move.c \
	sources/uci/uci_thread.c \
	sources/uci/uci_position.c \
	sources/uci/uci_isready.c \
	sources/uci/uci_ucinewgame.c \
	sources/uci/uci_go.c \
	sources/uci/uci_d.c \
	sources/uci/uci_quit.c \
	sources/uci/uci_uci.c \
	sources/uci/uci_setoption.c \
	sources/uci/uci_stop.c \
	sources/uci/uci_debug.c

OBJECTS	:= $(SOURCES:sources/%.c=objects/%.o)
DEPENDS	:= $(SOURCES:sources/%.c=objects/%.d)

WFLAGS	:= -Wall -Wextra
OFLAGS	:= -O3 -march=native
EXT_OFLAGS?=
DFLAGS	:= #-g3 #-fsanitize=address
LFLAGS	:= -lpthread -lm
EXT_LFLAGS?=

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(DFLAGS) $(LFLAGS) $(EXT_LFLAGS)

objects/%.o: sources/%.c Makefile
	@if test ! -d $(dir $@); then mkdir -p $(dir $@); fi
	$(CC) $(WFLAGS) $(OFLAGS) $(EXT_OFLAGS) $(DFLAGS) \
		-c -MMD -I include -o $@ $<

-include $(DEPENDS)

clean:
	rm -rf objects

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

