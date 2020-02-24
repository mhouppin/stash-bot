# **************************************************************************** #
#                                                           LE - /             #
#                                                               /              #
#    Makefile                                         .::    .:/ .      .::    #
#                                                  +:+:+   +:    +:  +:+:+     #
#    By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+      #
#                                                  #+#   #+    #+    #+#       #
#    Created: 2020/02/19 11:07:40 by stash        #+#   ##    ##    #+#        #
#    Updated: 2020/02/24 13:30:18 by stash       ###    #+. /#+    ###.fr      #
#                                                          /                   #
#                                                         /                    #
# **************************************************************************** #

NAME := stash-bot

SOURCES	:= $(wildcard sources/*/*.c)
OBJECTS	:= $(SOURCES:sources/%.c=objects/%.o)
DEPENDS := $(SOURCES:sources/%.c=objects/%.d)

OFLAGS	:= -O3

EXT_OFLAGS	?=
EXT_LFLAGS	?=

ifeq ($(ARCH),x86-64)
	OFLAGS += -DUSE_PREFETCH -msse
endif

ifeq ($(ARCH),x86-64-modern)
	OFLAGS += -DUSE_PREFETCH -msse
	OFLAGS += -DUSE_POPCNT -msse3 -mpopcnt
endif

ifeq ($(ARCH),x86-64-bmi2)
	OFLAGS += -DUSE_PREFETCH -msse
	OFLAGS += -DUSE_POPCNT -msse3 -mpopcnt
	OFLAGS += -DUSE_PEXT -msse4 -mbmi2
endif

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(OFLAGS) -o $@ $^ -lpthread -lm $(EXT_OFLAGS) $(EXT_LFLAGS)

objects/%.o: sources/%.c
	@if test ! -d $(dir $@); then mkdir -p $(dir $@); fi
	$(CC) $(OFLAGS) $(EXT_OFLAGS) -Wall -Wextra -Werror -c -MMD -I include \
		-o $@ $<

-include $(DEPENDS)

clean:
	rm -f $(OBJECTS)
	rm -f $(DEPENDS)
	rm -rf objects

fclean: clean
	rm -f $(NAME)

re: fclean all
