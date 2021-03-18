#
#    Stash, a UCI chess playing engine developed from scratch
#    Copyright (C) 2019-2021 Morgan Houppin
#
#    Stash is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Stash is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SOURCES := $(wildcard sources/*/*.c)

OBJECTS := $(SOURCES:%.c=%.o)
DEPENDS := $(SOURCES:%.c=%.d)
native = no

CFLAGS += -Wall -Wextra -Wcast-qual -Wshadow -Werror -O3 -flto
CPPFLAGS += -MMD -I include
LDFLAGS += -lpthread -lm

# If no arch is specified, we select a prefetch+popcnt build by default.
# If you want to build on a different architecture, please specify it in the
# command with ARCH=something. If you want to disable all arch-specific
# optimizations, use ARCH=unknown.

ifeq ($(ARCH),)
	ARCH=x86-64-modern
endif

# Check if CC is correctly pointing to a working compiler, and if not, set it
# to gcc

ifeq ($(shell $(CC) --version),)
	CC=gcc
endif

# Add .exe to the executable name if we are on Windows, and change the cleanup
# commands

ifeq ($(OS),Windows_NT)
	EXE = stash-bot.exe
	REMOVE = del
	SILENCE = >nul 2>&1 || (exit 0)
else
	EXE = stash-bot
	REMOVE = rm -f
endif

# Enable use of PREFETCH instruction

ifeq ($(ARCH),x86-64)
    CFLAGS += -DUSE_PREFETCH
    ifneq ($(native),yes)
        CFLAGS += -msse
    endif
endif

ifeq ($(ARCH),x86-64-modern)
    CFLAGS += -DUSE_PREFETCH -DUSE_POPCNT
    ifneq ($(native),yes)
        CFLAGS += -msse -msse3 -mpopcnt
    endif
endif

ifeq ($(ARCH),x86-64-bmi2)
    CFLAGS += -DUSE_PREFETCH -DUSE_POPCNT -DUSE_PEXT
    ifneq ($(native),yes)
        CFLAGS += -msse -msse3 -mpopcnt -msse4 -mbmi2
    endif
endif

# If native is specified, build will try to use all available CPU instructions

ifeq ($(native),yes)
    CFLAGS += -march=native
endif

all: $(EXE)

$(EXE): $(OBJECTS)
	+$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

-include $(DEPENDS)

clean:
	$(REMOVE) $(OBJECTS) $(DEPENDS) $(SILENCE)

fclean: clean
	$(REMOVE) $(EXE) $(SILENCE)

re:
	+$(MAKE) fclean
	+$(MAKE) all

.PHONY: all clean fclean re
