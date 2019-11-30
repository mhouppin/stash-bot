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

SOURCES	:= $(wildcard sources/*/*.c)
OBJECTS	:= $(patsubst sources/%.c,objects/%.o,$(SOURCES))
DEPENDS	:= $(patsubst sources/%.c,objects/%.d,$(SOURCES))

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
