# **************************************************************************** #
#                                                           LE - /             #
#                                                               /              #
#    Makefile                                         .::    .:/ .      .::    #
#                                                  +:+:+   +:    +:  +:+:+     #
#    By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+      #
#                                                  #+#   #+    #+    #+#       #
#    Created: 2019/10/28 13:18:56 by mhouppin     #+#   ##    ##    #+#        #
#    Updated: 2019/10/28 13:28:37 by mhouppin    ###    #+. /#+    ###.fr      #
#                                                          /                   #
#                                                         /                    #
# **************************************************************************** #

NAME	:= stash-bot

SOURCES	:= $(wildcard sources/*/*.c)
OBJECTS	:= $(patsubst sources/%.c,objects/%.o,$(SOURCES))
DEPENDS	:= $(patsubst sources/%.c,objects/%.d,$(SOURCES))

WFLAGS	:= -Wall -Wextra -Wpedantic
OFLAGS	:= -O2
DFLAGS	:=
LFLAGS	:= -lpthread -lm

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS)

objects/%.o: sources/%.c
	@if test ! -d $(dir $@); then mkdir -p $(dir $@); fi
	$(CC) $(WFLAGS) $(OFLAGS) $(DFLAGS) -c -MMD -I include -o $@ $<

-include $(DEPENDS)

clean:
	rm -rf objects

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
