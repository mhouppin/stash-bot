#
#	Stash, a UCI chess playing engine developed from scratch
#	Copyright (C) 2019-2020 Morgan Houppin
#
#	Stash is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	Stash is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

ifeq ($(EXE),)
EXE=stash-bot
endif

ifeq ($(OS),Windows_NT)
	REMOVE := del
	SILENCE := >nul 2>&1
	ifeq ($(EXE),)
		EXE=stash-bot.exe
	endif
else
	REMOVE := rm -f
	SILENCE := >/dev/null 2>&1
	ifeq ($(EXE),)
		EXE=stash-bot
	endif
endif

SOURCES	:= $(wildcard sources/*/*.c)
OBJECTS	:= $(SOURCES:%.c=%.o)
DEPENDS := $(SOURCES:%.c=%.d)

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

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(OFLAGS) -o $@ $^ -lpthread -lm $(EXT_OFLAGS) $(EXT_LFLAGS)

%.o: %.c
	$(CC) $(OFLAGS) $(EXT_OFLAGS) -Wall -Wextra -Werror -c -MMD -I include \
		-o $@ $<

-include $(DEPENDS)

clean:
	$(REMOVE) $(OBJECTS) $(SILENCE)
	$(REMOVE) $(DEPENDS) $(SILENCE)

re: clean all
